#include "Common.hlsl"

// =============================================================================
// DaftPunk.hlsl - Daft Punk Layout (helmet homage)
// Two half-helmets glide in from the sides and lock into one composite face, hold,
// then drift apart again:
//   left  = silver helmet: domed top on a wide brim band, thin wraparound black visor
//           slit, faceted jaw with a chin plate and mouth slot
//   right = gold helmet: glossy black glass dome and face, gold collar, side frames
//           and ear panels
// The cycle is 32 beats long (CustomParam7 = beat phase from main.cpp) and a detected
// drop slams the halves together. The visors double as mirrored LED equalizers.
// =============================================================================

float Sq(float x)
{
    return x * x;
}

float SmoothMin(float a, float b, float k)
{
    float h = saturate(0.5 + 0.5 * (b - a) / k);
    return lerp(b, a, h) - k * h * (1.0 - h);
}

float SdEllipsoid(float3 p, float3 r)
{
    float k0 = length(p / r);
    float k1 = length(p / (r * r));
    return k0 * (k0 - 1.0) / max(k1, 0.0001);
}

float SdRoundBox(float3 p, float3 b, float r)
{
    return SDFBox(p, b) - r;
}

// Elliptical (in xz) horizontal slab with rounded edges: wraparound bands, collars, rounded shells
float SdEllipticSlab(float3 p, float2 rxz, float yCenter, float halfH, float edge)
{
    float dxz = (length(p.xz / rxz) - 1.0) * min(rxz.x, rxz.y);
    float2 w = float2(dxz, abs(p.y - yCenter) - halfH) + edge;
    return min(max(w.x, w.y), 0.0) + length(max(w, 0.0)) - edge;
}

float SdCylinderY(float3 p, float h, float r)
{
    float2 d = abs(float2(length(p.xz), p.y)) - float2(r, h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float3 RotY(float3 p, float a)
{
    float c = cos(a);
    float s = sin(a);
    return float3(c * p.x + s * p.z, p.y, -s * p.x + c * p.z);
}

float3 CameraRay(float3 ro, float3 ta, float2 uv, float focal)
{
    float3 ww = normalize(ta - ro);
    float3 uu = normalize(cross(float3(0.0, 1.0, 0.0), ww));
    float3 vv = cross(ww, uu);
    return normalize(uv.x * uu + uv.y * vv + focal * ww);
}

// -----------------------------------------------------------------------------
// 1. Helmet Geometry (local frame: face looks toward -Z, head center at origin)
// Material ids: 0 silver, 1 gold, 4 silver-side visor glass, 5 gold-side glass, 6 dark
// -----------------------------------------------------------------------------

// Faceted lower face of the silver helmet: a convex block cut by tapered planes
float SilverJaw(float3 p)
{
    float3 q = float3(abs(p.x), p.y, p.z);
    float top = p.y + 0.25;                                        // tucks up under the cheek rim
    float bottom = -1.0 - p.y;
    float back = p.z - 0.6;
    float front = -(p.z + 0.92);
    float3 nSide = normalize(float3(1.0, -0.754, 0.0));
    float side = dot(q - float3(0.88, -0.31, 0.0), nSide);         // 0.88 wide under the visor, 0.36 at the chin
    float3 nCorner = normalize(float3(0.8, 0.0, -0.6));
    float corner = dot(q - float3(0.32, 0.0, -0.92), nCorner);     // flat chin plate with angled cheek facets
    float3 nChin = normalize(float3(0.0, -0.8, -0.6));
    float chin = dot(p - float3(0.0, -0.85, -0.92), nChin);        // beveled bottom edge of the chin
    float d = max(max(max(top, bottom), max(back, front)), max(max(side, corner), chin));

    // Small rectangular mouth slot in the chin plate
    float slot = SDFBox(p - float3(0.0, -0.68, -0.92), float3(0.14, 0.035, 0.06));
    return max(d, -slot);
}

float2 SilverHelmet(float3 p)
{
    // Rounded cap sitting just inside the brow rim
    float dome = SdEllipsoid(p - float3(0.0, 0.42, 0.05), float3(0.86, 0.66, 0.92));
    dome = max(dome, 0.44 - p.y);

    // Wraparound visor housing (brow rim above, cheek rim below) with a channel for the glass
    float band = SdEllipticSlab(p, float2(0.95, 1.05), 0.075, 0.38, 0.06);
    float channel = SDFBox(p - float3(0.0, 0.0, -0.62), float3(1.3, 0.23, 0.62));
    float housing = max(band, -channel);
    float glass = SdEllipticSlab(p, float2(0.9, 0.99), 0.0, 0.235, 0.03);

    // Rectangular ear blocks on the sides of the housing
    float3 q = float3(abs(p.x), p.y, p.z);
    float ear = SdRoundBox(q - float3(0.96, 0.0, 0.05), float3(0.04, 0.2, 0.22), 0.03);

    float metal = min(min(dome, housing), min(SilverJaw(p), ear));
    float neck = SdCylinderY(p - float3(0.0, -1.2, 0.15), 0.25, 0.42);

    float2 res = float2(metal, 0.0);
    if (glass < res.x) res = float2(glass, 4.0);
    if (neck < res.x) res = float2(neck, 6.0);
    return res;
}

float2 GoldHelmet(float3 p)
{
    // Rounded shell: the whole front and top is glossy black glass
    float3 pc = p - float3(0.0, 0.0, 0.05);
    float dome = SdEllipsoid(pc - float3(0.0, 0.05, 0.0), float3(0.93, 1.02, 1.0));
    float lower = SdEllipticSlab(pc, float2(0.88, 0.96), -0.45, 0.45, 0.08);
    float shell = SmoothMin(dome, lower, 0.2);

    // Rounded gold collar wrapping the bottom and slim gold ear panels on the sides
    float collar = SdEllipticSlab(pc, float2(0.94, 1.02), -0.82, 0.19, 0.06);
    float3 q = float3(abs(p.x), p.y, p.z);
    float ear = SdRoundBox(q - float3(0.93, -0.25, 0.08), float3(0.04, 0.3, 0.24), 0.03);
    float frame = min(collar, ear);
    float neck = SdCylinderY(p - float3(0.0, -1.22, 0.15), 0.2, 0.42);

    float2 res = float2(shell, 5.0);
    if (frame < res.x) res = float2(frame, 1.0);
    if (neck < res.x) res = float2(neck, 6.0);
    return res;
}

// Distance + material; cut faces: 2 = silver half's inside, 3 = gold half's inside
float2 MapHelmets(float3 p, float gap, float yaw)
{
    float3 pl = RotY(p + float3(gap, 0.0, 0.0), yaw);
    float3 pr = RotY(p - float3(gap, 0.0, 0.0), -yaw);
    float2 silver = SilverHelmet(pl);
    float2 gold = GoldHelmet(pr);
    float left = max(silver.x, pl.x);    // keep the left half of the silver helmet
    float right = max(gold.x, -pr.x);    // keep the right half of the gold helmet
    if (left < right) return float2(left, (pl.x > silver.x) ? 2.0 : silver.y);
    return float2(right, (-pr.x > gold.x) ? 3.0 : gold.y);
}

// -----------------------------------------------------------------------------
// 2. Studio Lighting (what the metal and glass reflect)
// -----------------------------------------------------------------------------
float3 StudioEnv(float3 r)
{
    float3 col = float3(0.03, 0.032, 0.045) + float3(0.12, 0.13, 0.17) * saturate(r.y * 0.6 + 0.4);

    // Overhead softbox, a front fill panel and two tall side strips
    col += float3(1.0, 0.98, 0.95) * smoothstep(0.7, 0.9, r.y) * 2.2;
    col += float3(0.9, 0.9, 1.0) * exp(-Sq(r.x / 0.35) - Sq((r.y - 0.25) / 0.2)) * step(r.z, 0.0) * 1.2;
    float strip = smoothstep(-0.35, 0.1, r.y) * (1.0 - smoothstep(0.55, 0.85, r.y));
    col += float3(1.0, 0.95, 0.9) * exp(-Sq((r.x - 0.75) / 0.08)) * strip * 2.8;
    col += ColorSecondary.rgb * exp(-Sq((r.x + 0.75) / 0.1)) * strip * 2.2;

    // Colored stage lights sweeping around the room in time with the beat
    float az = atan2(r.x, r.z);
    float sweep = pow(saturate(sin(az * 3.0 + CustomParam7 * 1.5708)), 24.0);
    col += lerp(ColorPrimary.rgb, ColorAccent.rgb, 0.5 + 0.5 * sin(az)) * sweep * smoothstep(-0.1, 0.4, r.y) * (0.6 + AudioKick * 0.8);

    // Dark stage floor with a warm horizon glow
    col += float3(0.3, 0.2, 0.12) * exp(-abs(r.y + 0.05) * 25.0) * 0.4;
    return col;
}

// Satin (brushed) metal: diffuse studio light plus a softened reflection
float3 SatinMetal(float3 albedo, float3 n, float3 rd)
{
    float3 refl = reflect(rd, n);
    float3 keyDir = normalize(float3(-0.5, 0.7, -0.5));
    float3 fillDir = normalize(float3(0.6, 0.2, -0.6));
    float diff = saturate(dot(n, keyDir)) * 0.9 + saturate(dot(n, fillDir)) * 0.35 + 0.08;
    float3 env = StudioEnv(normalize(lerp(refl, n, 0.55)));
    float spec = pow(saturate(dot(refl, keyDir)), 24.0) * 0.5;
    return albedo * diff * 0.95 + env * albedo * 0.35 + lerp(albedo, float3(1.0, 1.0, 1.0), 0.5) * spec;
}

// Glossy black glass with sharp studio highlights
float3 BlackGlass(float3 n, float3 rd)
{
    float3 refl = reflect(rd, n);
    float fres = pow(1.0 - saturate(dot(n, -rd)), 5.0);
    float3 keyDir = normalize(float3(-0.5, 0.7, -0.5));
    float spec = pow(saturate(dot(refl, keyDir)), 200.0) * 1.5;
    return float3(0.004, 0.005, 0.008) + StudioEnv(refl) * (0.06 + 0.7 * fres) + float3(1.0, 1.0, 1.0) * spec;
}

// -----------------------------------------------------------------------------
// 3. Visor LED Equalizer & Cut-Face Circuitry
// -----------------------------------------------------------------------------
// Mirrored spectrum: bass at the seam, highs toward the edges. The bars fill the
// visor from yBottom to yTop (red LEDs on the silver side, rainbow on the gold side).
float3 VisorLeds(float2 q, bool goldSide, float yBottom, float yTop, float halfWidth)
{
    float cell = goldSide ? 0.055 : 0.045;
    float2 id = floor(q / cell);
    float2 f = frac(q / cell) - 0.5;
    float dotMask = smoothstep(0.42, 0.22, length(f));

    float colN = saturate((abs(id.x) + 0.5) * cell / halfWidth);
    float level = GetAudioFFT(0.02 + colN * 0.5) * 1.15;
    float rowN = ((id.y + 0.5) * cell - yBottom) / (yTop - yBottom);
    float inside = step(0.0, rowN) * step(rowN, 1.0) * step(abs(q.x), halfWidth);
    float lit = step(rowN, level);
    float peak = step(abs(rowN - level), cell / (yTop - yBottom)) * 0.6; // brighter cap on each bar

    float3 ledCol = goldSide ? HSVtoRGB(float3(colN * 0.8, 0.9, 1.0)) : float3(1.0, 0.12, 0.05);
    return ledCol * dotMask * (lit * 1.6 + peak + 0.04) * inside;
}

float3 CutFaceCircuit(float2 q, float join)
{
    // Glowing traces and a pulsing core on the exposed inside of each half
    float2 g = abs(frac(q * 7.0) - 0.5);
    float2 cellId = floor(q * 7.0);
    float trace = (Hash21(cellId) > 0.45) ? (1.0 - smoothstep(0.0, 0.06, g.x)) : (1.0 - smoothstep(0.0, 0.06, g.y));
    float core = exp(-Sq(length(q - float2(0.15, 0.0)) / 0.25));
    float pulse = 0.5 + AudioKick * 0.9 + BeatPulse * 0.5 * SafetyDamping;
    float3 col = float3(0.02, 0.022, 0.03);
    col += ColorSecondary.rgb * trace * 0.8 * pulse;
    col += lerp(ColorSecondary.rgb, float3(1.0, 1.0, 1.0), 0.4) * core * (0.8 + AudioSubBass * 1.2);
    return col * (1.0 - join * 0.7);
}

// -----------------------------------------------------------------------------
// 4. Main Pixel Shader
// -----------------------------------------------------------------------------
float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;

    // 32-beat cycle: apart (0-4), glide together and lock (4-8), hold (8-28), drift apart (28-31.5)
    float cyc = fmod(CustomParam7, 32.0);
    float glide = pow(saturate((cyc - 4.0) / 4.0), 2.2);   // accelerates into the lock
    float release = smoothstep(28.0, 31.5, cyc);
    float join = glide * (1.0 - release);
    join = max(join, DropIntensity * SafetyDamping);       // a drop slams the halves together
    float gap = lerp(1.3, 0.0, join) + AudioSubBass * 0.012 * join; // the seam breathes with the sub-bass
    float yaw = -0.22 * (1.0 - join);
    float since = cyc - 8.0;
    float lockFlash = (since >= 0.0 && since < 4.0) ? exp(-since * 2.5) * SafetyDamping : 0.0;

    float camYaw = 0.22 * sin(Time * 0.15);
    float3 ro = float3(sin(camYaw) * 4.7, 0.2 + 0.15 * sin(Time * 0.11), -cos(camYaw) * 4.7);
    ro.x += lockFlash * 0.015 * sin(Time * 70.0); // tiny camera kick on the lock
    float3 rd = CameraRay(ro, float3(0.0, -0.1, 0.0), uv, 2.2);

    // Backdrop: dark stage, soft spotlight behind the helmets, sweeping beams
    float3 col = float3(0.004, 0.005, 0.01) + float3(0.07, 0.06, 0.09) * exp(-length(uv - float2(0.0, 0.15)) * 1.3) * (1.0 + AudioSubBass * 0.5);
    for (int b = 0; b < 2; ++b)
    {
        float side = (b == 0) ? -1.0 : 1.0;
        float bx = side * (0.9 + 0.5 * sin(CustomParam7 * 0.785 + float(b) * 1.7));
        float beam = exp(-Sq((uv.x - bx - (uv.y - 1.0) * side * 0.35) / (0.08 + (1.0 - uv.y) * 0.12)));
        col += ((b == 0) ? ColorSecondary.rgb : ColorPrimary.rgb) * beam * smoothstep(-1.0, 1.0, uv.y) * 0.12 * (0.7 + AudioKick * 0.6);
    }

    // Raymarch the two halves
    float t = 2.5;
    float2 hit = float2(0.0, -1.0);
    for (int i = 0; i < 120; ++i)
    {
        float2 m = MapHelmets(ro + rd * t, gap, yaw);
        if (m.x < 0.0008 * t)
        {
            hit = float2(t, m.y);
            break;
        }
        t += m.x * 0.85;
        if (t > 10.0) break;
    }

    if (hit.y >= 0.0)
    {
        float3 p = ro + rd * hit.x;
        float e = 0.0015;
        float3 n = normalize(
            float3( 1.0, -1.0, -1.0) * MapHelmets(p + float3( e, -e, -e), gap, yaw).x +
            float3(-1.0, -1.0,  1.0) * MapHelmets(p + float3(-e, -e,  e), gap, yaw).x +
            float3(-1.0,  1.0, -1.0) * MapHelmets(p + float3(-e,  e, -e), gap, yaw).x +
            float3( 1.0,  1.0,  1.0) * MapHelmets(p + float3( e,  e,  e), gap, yaw).x);

        int mat = int(hit.y + 0.5);
        bool goldSide = (mat == 1 || mat == 3 || mat == 5);
        float3 lp = goldSide ? RotY(p - float3(gap, 0.0, 0.0), -yaw) : RotY(p + float3(gap, 0.0, 0.0), yaw); // helmet-local position
        float3 silverAlbedo = float3(0.72, 0.74, 0.78);
        float3 goldAlbedo = float3(0.95, 0.68, 0.22);

        if (mat == 2 || mat == 3)
        {
            // Exposed cut face (only visible while the halves are apart)
            col = CutFaceCircuit(lp.yz, join);
        }
        else if (mat == 4)
        {
            // Thin wraparound visor slit on the silver side
            col = BlackGlass(n, rd) + VisorLeds(lp.xy, false, -0.2, 0.2, 0.9) * smoothstep(-0.3, -0.7, lp.z);
        }
        else if (mat == 5)
        {
            // Gold side: black glass dome and face, framed in gold strips along the sides
            float sideFrame = step(0.74, abs(lp.x)) * step(lp.y, 0.42);
            col = (sideFrame > 0.5) ? SatinMetal(goldAlbedo, n, rd)
                                    : BlackGlass(n, rd) + VisorLeds(lp.xy, true, -0.55, 0.45, 0.68) * smoothstep(-0.3, -0.7, lp.z);
        }
        else if (mat == 6)
        {
            col = float3(0.01, 0.01, 0.012) + StudioEnv(reflect(rd, n)) * 0.03; // dark collar / neck
        }
        else
        {
            col = SatinMetal(goldSide ? goldAlbedo : silverAlbedo, n, rd);
            if (!goldSide)
            {
                // Mouth slot and the row of slits under the visor read as dark cut-outs
                float slot = step(abs(lp.x), 0.15) * step(abs(lp.y + 0.68), 0.04) * step(lp.z, -0.8);
                float slits = step(abs(lp.y + 0.27), 0.018) * step(abs(lp.x), 0.6) * step(frac(lp.x * 6.0 + 0.5), 0.7) * step(lp.z, -0.95);
                col *= 1.0 - max(slot, slits) * 0.9;
            }
        }

        // Seam glows when the halves lock together
        col += lerp(float3(1.0, 0.9, 0.6), ColorAccent.rgb, 0.3) * exp(-abs(lp.x) / 0.02) * (lockFlash * 2.0 + join * AudioSubBass * 0.3);
    }

    // Lock shockwave ring around the face
    col += float3(1.0, 0.9, 0.7) * exp(-Sq((length(uv) - (1.0 - lockFlash) * 1.6) / 0.05)) * lockFlash * 0.6;

    // Soft tone curve, then kid-safe brightness cap
    col = 1.0 - exp(-col * 1.25);
    col *= MasterBrightness;

    // Shared vignette layer (mixer "Vignette & Drop Pulse")
    if (SpookyWeight > 0.01)
    {
        float edgeDist = length(input.UV - 0.5) * 2.0;
        col *= clamp(1.0 - smoothstep(0.7, 1.4, edgeDist) * 0.45 * SpookyWeight, 0.0, 1.0);
    }

    return float4(saturate(col), 1.0);
}
