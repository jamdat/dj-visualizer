"""Generate the DAD PUNK hero 3D mesh from a TrueType font.

Pipeline: font glyph outlines -> flattened polygons (with holes) -> earcut
triangulated caps + vertical walls + chamfer bevels with smooth normals ->
OBJ and fast binary (.bin) used by src/graphics/Mesh3D.cpp.

Requires: pip install fonttools mapbox-earcut numpy

Usage: python scripts/generate_dad_punk_3d.py [TEXT]
"""
import math
import os
import struct
import sys

import numpy as np
import mapbox_earcut as earcut
from fontTools.pens.basePen import BasePen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.ttLib import TTFont

FONT_PATH = 'assets/fonts/Daft Font.TTF'
DEFAULT_TEXT = 'DAD PUNK'

# World-space layout. Total mesh width in world units (height follows font aspect).
TARGET_WIDTH = 4.0
HALF_DEPTH = 0.12     # half of total extrusion thickness
BEVEL_DEPTH = 0.02    # how far the bevel steps back from the cap (z)
BEVEL_INSET = 0.012   # how far the cap is inset from the outline (xy)
INSET_FEATURE_FRACTION = 0.4  # inset never exceeds this fraction of local feature size (stops flips in thin parts)
MITER_LIMIT = 1.5
SIMPLIFY_EPS = 1.0    # Douglas-Peucker tolerance in font units (removes micro-edges)
GLYPH_GAP = 40        # font units between glyph bounding boxes (stops overlap blobs)
SPACE_WIDTH = 260     # font units for a space
CURVE_STEPS = 8
SMOOTH_ANGLE_DEG = 40.0


class FlattenPen(BasePen):
    """Collects glyph contours as flattened polylines."""

    def __init__(self, glyph_set):
        super().__init__(glyph_set)
        self.contours = []
        self._cur = None

    def _moveTo(self, pt):
        self._cur = [pt]

    def _lineTo(self, pt):
        self._cur.append(pt)

    def _curveToOne(self, p1, p2, p3):
        p0 = self._cur[-1]
        for i in range(1, CURVE_STEPS + 1):
            t = i / CURVE_STEPS
            mt = 1 - t
            x = mt**3 * p0[0] + 3 * mt * mt * t * p1[0] + 3 * mt * t * t * p2[0] + t**3 * p3[0]
            y = mt**3 * p0[1] + 3 * mt * mt * t * p1[1] + 3 * mt * t * t * p2[1] + t**3 * p3[1]
            self._cur.append((x, y))

    def _qCurveToOne(self, p1, p2):
        p0 = self._cur[-1]
        for i in range(1, CURVE_STEPS + 1):
            t = i / CURVE_STEPS
            mt = 1 - t
            x = mt * mt * p0[0] + 2 * mt * t * p1[0] + t * t * p2[0]
            y = mt * mt * p0[1] + 2 * mt * t * p1[1] + t * t * p2[1]
            self._cur.append((x, y))

    def _closePath(self):
        if self._cur and len(self._cur) >= 3:
            self.contours.append(self._cur)
        self._cur = None

    def _endPath(self):
        self._closePath()


def signed_area(poly):
    a = 0.0
    n = len(poly)
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        a += x0 * y1 - x1 * y0
    return a * 0.5


def simplify_ring(poly, eps):
    """Douglas-Peucker on a closed ring (split at the two farthest-apart anchor points)."""
    n = len(poly)
    if n <= 4 or eps <= 0:
        return poly
    pts = np.array(poly, dtype=np.float64)
    i0 = 0
    i1 = int(np.argmax(np.linalg.norm(pts - pts[0], axis=1)))

    def dp(lo, hi, idxs):
        # idxs: list of ring indices from lo..hi inclusive (open chain)
        keep = {idxs[0], idxs[-1]}
        stack = [(0, len(idxs) - 1)]
        while stack:
            s, e = stack.pop()
            if e <= s + 1:
                continue
            a, b = pts[idxs[s]], pts[idxs[e]]
            ab = b - a
            l2 = max(float(ab @ ab), 1e-18)
            seg = pts[idxs[s + 1:e]]
            t = np.clip(((seg - a) @ ab) / l2, 0.0, 1.0)
            d = np.linalg.norm(seg - (a + t[:, None] * ab), axis=1)
            k = int(np.argmax(d))
            if d[k] > eps:
                m = s + 1 + k
                keep.add(idxs[m])
                stack.append((s, m))
                stack.append((m, e))
        return keep

    chain1 = list(range(i0, i1 + 1))
    chain2 = list(range(i1, n)) + [0]
    keep = dp(i0, i1, chain1) | dp(i1, n, chain2)
    out = [poly[i] for i in range(n) if i in keep]
    return out if len(out) >= 3 else poly


def clean(poly, eps=1e-4):
    out = []
    for p in poly:
        if not out or math.hypot(p[0] - out[-1][0], p[1] - out[-1][1]) > eps:
            out.append(p)
    if len(out) > 1 and math.hypot(out[0][0] - out[-1][0], out[0][1] - out[-1][1]) <= eps:
        out.pop()
    return simplify_ring(out, SIMPLIFY_EPS)


def point_in_poly(pt, poly):
    x, y = pt
    inside = False
    n = len(poly)
    j = n - 1
    for i in range(n):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi + 1e-30) + xi:
            inside = not inside
        j = i
    return inside


def glyph_shapes(contours):
    """Group contours into [(outer, [holes])]. Outer = same winding as the largest contour."""
    polys = [clean(c) for c in contours]
    polys = [p for p in polys if len(p) >= 3]
    if not polys:
        return []
    ref_sign = math.copysign(1.0, signed_area(max(polys, key=lambda p: abs(signed_area(p)))))
    outers, holes = [], []
    for p in polys:
        (outers if math.copysign(1.0, signed_area(p)) == ref_sign else holes).append(p)
    shapes = [(o, []) for o in outers]
    for h in holes:
        best = None
        for o, hl in shapes:
            if point_in_poly(h[0], o):
                if best is None or abs(signed_area(o)) < abs(signed_area(best[0])):
                    best = (o, hl)
        if best is not None:
            best[1].append(h)
    return shapes


def orient(poly, ccw):
    return poly if (signed_area(poly) > 0) == ccw else poly[::-1]


def edge_normals(poly):
    """Outward-from-material normal per edge (material is on the left of travel)."""
    n = len(poly)
    out = []
    for i in range(n):
        x0, y0 = poly[i]
        x1, y1 = poly[(i + 1) % n]
        dx, dy = x1 - x0, y1 - y0
        ln = math.hypot(dx, dy) or 1.0
        out.append((dy / ln, -dx / ln))
    return out


def vertex_normals(poly, en):
    """Per-edge-end normals: returns (start_normals, end_normals) with smooth/hard split."""
    n = len(poly)
    cos_t = math.cos(math.radians(SMOOTH_ANGLE_DEG))
    start, end = [], []
    for i in range(n):
        cur = en[i]
        prev = en[(i - 1) % n]
        nxt = en[(i + 1) % n]

        def blend(a, b):
            if a[0] * b[0] + a[1] * b[1] >= cos_t:
                m = (a[0] + b[0], a[1] + b[1])
                ln = math.hypot(*m) or 1.0
                return (m[0] / ln, m[1] / ln)
            return a

        start.append(blend(cur, prev))
        end.append(blend(cur, nxt))
    return start, end


def adaptive_inset(ring, all_rings):
    """Per-vertex inset distance: BEVEL_INSET, clamped to a fraction of the distance to the
    nearest non-neighbouring edge (same shape), so thin features cannot self-intersect."""
    pts = np.array(ring, dtype=np.float64)
    n = len(pts)
    segs_a, segs_b, owner, eidx = [], [], [], []
    for r_idx, r in enumerate(all_rings):
        arr = np.array(r, dtype=np.float64)
        segs_a.append(arr)
        segs_b.append(np.roll(arr, -1, axis=0))
        owner.extend([r_idx] * len(arr))
        eidx.extend(range(len(arr)))
    A = np.concatenate(segs_a)
    B = np.concatenate(segs_b)
    owner = np.array(owner)
    eidx = np.array(eidx)
    own_idx = next(i for i, r in enumerate(all_rings) if r is ring)
    AB = B - A
    ab2 = np.maximum((AB * AB).sum(1), 1e-18)
    out = []
    for i in range(n):
        p = pts[i]
        t = np.clip(((p - A) * AB).sum(1) / ab2, 0.0, 1.0)
        d = np.linalg.norm(p - (A + AB * t[:, None]), axis=1)
        # ignore edges close to this vertex along its own ring
        diff = (eidx - i) % n
        near = (owner == own_idx) & ((diff <= 2) | (diff >= n - 3))
        d[near] = np.inf
        feature = d.min()
        out.append(min(BEVEL_INSET, INSET_FEATURE_FRACTION * feature))
    return out


def inset_poly(poly, en, dists):
    """Move each vertex toward the material by its own `dists[i]` using a clamped miter."""
    n = len(poly)
    out = []
    for i in range(n):
        dist = dists[i]
        a = en[(i - 1) % n]
        b = en[i]
        mx, my = a[0] + b[0], a[1] + b[1]
        ln = math.hypot(mx, my)
        if ln < 1e-6:
            mx, my = b
            scale = 1.0
        else:
            mx, my = mx / ln, my / ln
            scale = min(1.0 / max(mx * b[0] + my * b[1], 1e-3), MITER_LIMIT)
        out.append((poly[i][0] - mx * dist * scale, poly[i][1] - my * dist * scale))
    return out


def build_layout(text, font):
    cmap = font.getBestCmap()
    gs = font.getGlyphSet()
    glyphs = []
    cursor = 0.0
    for ch in text:
        if ch == ' ':
            cursor += SPACE_WIDTH
            continue
        name = cmap.get(ord(ch)) or cmap.get(ord(ch.lower())) or cmap.get(ord(ch.upper()))
        if not name:
            print(f"warning: no glyph for {ch!r}, skipping")
            continue
        bp = BoundsPen(gs)
        gs[name].draw(bp)
        if bp.bounds is None:
            continue
        pen = FlattenPen(gs)
        gs[name].draw(pen)
        xmin, ymin, xmax, ymax = bp.bounds
        dx = cursor - xmin + (GLYPH_GAP if glyphs else 0)
        contours = [[(x + dx, y) for x, y in c] for c in pen.contours]
        glyphs.append(contours)
        cursor = xmax + dx
    return glyphs


def generate_mesh(text, output_obj):
    font = TTFont(FONT_PATH)
    glyphs = build_layout(text, font)

    shapes = []
    for contours in glyphs:
        shapes.extend(glyph_shapes(contours))

    allpts = [p for o, hl in shapes for poly in [o] + hl for p in poly]
    xs = [p[0] for p in allpts]
    ys = [p[1] for p in allpts]
    cx, cy = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2
    s = TARGET_WIDTH / (max(xs) - min(xs))
    height = (max(ys) - min(ys)) * s

    def to_world(poly):
        return [((x - cx) * s, (y - cy) * s) for x, y in poly]

    vertices, normals, uvs, barys, triangles = [], [], [], [], []

    def uv_of(p):
        return ((p[0] + TARGET_WIDTH / 2) / TARGET_WIDTH, (p[1] + height / 2) / height)

    def add_tri(p0, p1, p2, n0, n1, n2):
        base = len(vertices)
        vertices.extend([p0, p1, p2])
        normals.extend([n0, n1, n2])
        uvs.extend([uv_of(p0), uv_of(p1), uv_of(p2)])
        barys.extend([(1, 0, 0), (0, 1, 0), (0, 0, 1)])
        triangles.append((base + 1, base + 2, base + 3))

    def add_quad(p0, p1, p2, p3, n0, n1, n2, n3):
        add_tri(p0, p1, p2, n0, n1, n2)
        add_tri(p0, p2, p3, n0, n2, n3)

    w = HALF_DEPTH - BEVEL_DEPTH  # z of the vertical wall ends
    bz = BEVEL_DEPTH / (BEVEL_DEPTH + BEVEL_INSET)
    nz = BEVEL_INSET / math.hypot(BEVEL_DEPTH, BEVEL_INSET)  # bevel normal z component
    nxy = BEVEL_DEPTH / math.hypot(BEVEL_DEPTH, BEVEL_INSET)  # bevel normal xy magnitude
    del bz

    for outer_src, holes_src in shapes:
        rings = [orient(to_world(outer_src), True)] + [orient(to_world(h), False) for h in holes_src]
        inset_rings = []

        for ring in rings:
            en = edge_normals(ring)
            sn, ensn = vertex_normals(ring, en)
            inner = inset_poly(ring, en, adaptive_inset(ring, rings))
            inset_rings.append(inner)
            n = len(ring)
            for i in range(n):
                j = (i + 1) % n
                pi, pj = ring[i], ring[j]
                qi, qj = inner[i], inner[j]
                a, b = sn[i], ensn[i]  # normals at edge start / end (xy, outward)

                # Vertical wall
                n_a, n_b = (a[0], a[1], 0.0), (b[0], b[1], 0.0)
                add_quad((pi[0], pi[1], -w), (pj[0], pj[1], -w), (pj[0], pj[1], w), (pi[0], pi[1], w),
                         n_a, n_b, n_b, n_a)

                # Bevel normals blend outward xy with +/- z
                bf_a = (a[0] * nxy, a[1] * nxy, nz)
                bf_b = (b[0] * nxy, b[1] * nxy, nz)
                bb_a = (a[0] * nxy, a[1] * nxy, -nz)
                bb_b = (b[0] * nxy, b[1] * nxy, -nz)
                add_quad((pi[0], pi[1], w), (pj[0], pj[1], w), (qj[0], qj[1], HALF_DEPTH), (qi[0], qi[1], HALF_DEPTH),
                         bf_a, bf_b, bf_b, bf_a)
                add_quad((pj[0], pj[1], -w), (pi[0], pi[1], -w), (qi[0], qi[1], -HALF_DEPTH), (qj[0], qj[1], -HALF_DEPTH),
                         bb_b, bb_a, bb_a, bb_b)

        # Caps from inset rings, triangulated with holes
        flat = np.array([p for r in inset_rings for p in r], dtype=np.float64)
        ends = np.cumsum([len(r) for r in inset_rings]).astype(np.uint32)
        idx = earcut.triangulate_float64(flat, ends)
        for t in range(0, len(idx), 3):
            a, b, c = (tuple(flat[idx[t + k]]) for k in range(3))
            area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
            if area < 0:
                b, c = c, b
            add_tri((a[0], a[1], HALF_DEPTH), (b[0], b[1], HALF_DEPTH), (c[0], c[1], HALF_DEPTH),
                    (0.0, 0.0, 1.0), (0.0, 0.0, 1.0), (0.0, 0.0, 1.0))
            add_tri((a[0], a[1], -HALF_DEPTH), (c[0], c[1], -HALF_DEPTH), (b[0], b[1], -HALF_DEPTH),
                    (0.0, 0.0, -1.0), (0.0, 0.0, -1.0), (0.0, 0.0, -1.0))

    os.makedirs(os.path.dirname(output_obj), exist_ok=True)
    with open(output_obj, 'w', encoding='utf-8') as f:
        f.write(f"# '{text}' 3D polygonal mesh (font-based, beveled)\n")
        f.write(f"# Vertices: {len(vertices)}, Triangles: {len(triangles)}\n")
        for v in vertices:
            f.write(f"v {v[0]:.5f} {v[1]:.5f} {v[2]:.5f}\n")
        for n in normals:
            f.write(f"vn {n[0]:.4f} {n[1]:.4f} {n[2]:.4f}\n")
        for u in uvs:
            f.write(f"vt {u[0]:.4f} {u[1]:.4f}\n")
        for tri in triangles:
            f.write(f"f {tri[0]}/{tri[0]}/{tri[0]} {tri[1]}/{tri[1]}/{tri[1]} {tri[2]}/{tri[2]}/{tri[2]}\n")

    # Fast binary format for sub-millisecond loading
    bin_path = os.path.splitext(output_obj)[0] + ".bin"
    with open(bin_path, 'wb') as f:
        v_count = len(vertices)
        f.write(struct.pack('<II', v_count, len(triangles) * 3))
        # Vertex: pos(3f), norm(3f), uv(2f), bary(3f) = 11 floats
        for i in range(v_count):
            p, n, uv, b = vertices[i], normals[i], uvs[i], barys[i]
            f.write(struct.pack('<11f', p[0], p[1], p[2], n[0], n[1], n[2], uv[0], uv[1], b[0], b[1], b[2]))
        for tri in triangles:
            f.write(struct.pack('<3I', tri[0] - 1, tri[1] - 1, tri[2] - 1))

    print(f"Generated {output_obj} and {bin_path}: {len(vertices)} vertices, {len(triangles)} triangles, "
          f"size {TARGET_WIDTH:.2f} x {height:.2f} x {2 * HALF_DEPTH:.2f}")


if __name__ == '__main__':
    text = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_TEXT
    generate_mesh(text, 'assets/models/dad_punk_3d.obj')
    generate_mesh(text, 'build/bin/assets/models/dad_punk_3d.obj')
