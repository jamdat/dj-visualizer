#include "Common.hlsl"

// =============================================================================
// DanceParty.hlsl - Dance Party Layout
// Neon dancers wearing faces from the face atlas, dancing to the beat on a wet
// cyberpunk stage: layered neon skyline, equalizer billboards, flying-car trails,
// rain and beat lasers. Every 32 beats the dancers slide to new spots so each one
// gets a turn in the front row.
// CustomParam7 = beat phase (beats, wraps at 64) from main.cpp.
// =============================================================================

// Up to six 512x512 faces side by side (assets/private/faces_atlas.tga, git-ignored).
// When it's missing the renderer binds a transparent 1x1 texture and placeholder faces are drawn.
Texture2D FaceAtlasTexture : register(t7);

float Sq(float x)
{
    return x * x;
}

float SegDist(float2 p, float2 a, float2 b)
{
    float2 pa = p - a;
    float2 ba = b - a;
    float h = saturate(dot(pa, ba) / max(dot(ba, ba), 0.000001));
    return length(pa - ba * h);
}

float3 DancerColor(int i)
{
    if (i == 0) return float3(1.0, 0.2, 0.8);
    if (i == 1) return float3(0.1, 0.9, 1.0);
    if (i == 2) return float3(1.0, 0.9, 0.2);
    if (i == 3) return float3(0.4, 1.0, 0.3);
    if (i == 4) return float3(1.0, 0.5, 0.1);
    return float3(0.65, 0.45, 1.0);
}

// -----------------------------------------------------------------------------
// 1. Faces
// -----------------------------------------------------------------------------
// q = position inside the head circle, [-1, 1] with +y up
float4 SampleFace(int slot, float2 q, float lod)
{
    float2 fuv = float2((float(slot) + 0.5 + q.x * 0.5) / 6.0, 0.5 - q.y * 0.5);
    return FaceAtlasTexture.SampleLevel(SamplerLinearClamp, fuv, lod);
}

bool FacePresent(int slot)
{
    float2 center = float2((float(slot) + 0.5) / 6.0, 0.5);
    float4 c = FaceAtlasTexture.SampleLevel(SamplerLinearClamp, center, 2.0);
    return c.a > 0.5 && dot(c.rgb, float3(1.0, 1.0, 1.0)) > 0.05; // an unbound texture reads as opaque black
}

// Neon smiley used when no face atlas is installed
float4 PlaceholderFace(float2 q, float3 color)
{
    float r = length(q);
    float3 col = float3(0.05, 0.03, 0.08) + color * 0.15;
    float eyes = 1.0 - smoothstep(0.1, 0.14, min(length(q - float2(-0.32, 0.25)), length(q - float2(0.32, 0.25))));
    float smile = (1.0 - smoothstep(0.04, 0.08, abs(length(q - float2(0.0, 0.1)) - 0.5))) * step(q.y, -0.05);
    col += color * (eyes + smile) * 1.5;
    return float4(col, step(r, 1.0));
}

// -----------------------------------------------------------------------------
// 2. Dance Moves (local units: feet at the origin, about 1.1 tall including the head)
// -----------------------------------------------------------------------------
struct Pose
{
    float2 hip;
    float2 neck;
    float2 head;
    float2 shoulderL;
    float2 elbowL;
    float2 handL;
    float2 shoulderR;
    float2 elbowR;
    float2 handR;
    float2 kneeL;
    float2 footL;
    float2 kneeR;
    float2 footR;
    float tilt;
};

// Arm angles are measured from hanging straight down; positive swings outward and up
float2 ArmDir(float angle, float side)
{
    return float2(side * sin(angle), -cos(angle));
}

Pose DancePose(int move, float beat, float energy)
{
    float ph = frac(beat);
    float alt = (fmod(floor(beat), 2.0) < 1.0) ? 1.0 : -1.0; // flips every beat
    float pump = sin(ph * 6.2831853);
    float dip = (0.5 - 0.5 * cos(ph * 6.2831853)) * (0.7 + energy * 0.5); // knees dip on each beat
    float jump = 0.0;
    float sway = 0.0;
    float lean = 0.0;
    float spread = 0.0;
    float upL, foreL, upR, foreR;

    if (move == 0)
    {
        // Bounce and pump: fists punch up on alternating beats
        upL = 0.6 + 1.6 * max(pump * alt, 0.0);
        upR = 0.6 + 1.6 * max(-pump * alt, 0.0);
        foreL = 1.3 - 0.9 * max(pump * alt, 0.0);
        foreR = 1.3 - 0.9 * max(-pump * alt, 0.0);
        sway = 0.025 * pump;
    }
    else if (move == 1)
    {
        // Hands in the air, waving side to side
        float wave = sin(beat * 3.14159265);
        upL = 2.7 + 0.25 * wave;
        upR = 2.7 - 0.25 * wave;
        foreL = 0.35 * wave;
        foreR = -0.35 * wave;
        sway = 0.06 * wave;
        lean = 0.06 * wave;
    }
    else if (move == 2)
    {
        // Disco point: one arm points up, the other rests on the hip, switching every beat
        upL = (alt > 0.0) ? 2.5 : 0.8;
        foreL = (alt > 0.0) ? 0.05 : 2.3;
        upR = (alt > 0.0) ? 0.8 : 2.5;
        foreR = (alt > 0.0) ? 2.3 : 0.05;
        lean = -0.09 * alt;
        sway = 0.04 * alt;
    }
    else
    {
        // Star jump on every beat
        float air = sin(ph * 3.14159265);
        jump = 0.09 * air * (0.6 + energy * 0.6);
        upL = 0.5 + 1.9 * air;
        upR = upL;
        foreL = 0.15;
        foreR = 0.15;
        spread = 0.08 * air;
        dip *= 0.3;
    }

    Pose ps;
    ps.hip = float2(sway, 0.42 - dip * 0.05 + jump);
    ps.neck = ps.hip + Rotate2D(float2(0.0, 0.3), lean);
    ps.tilt = 0.18 * sin(beat * 3.14159265) + lean;
    ps.head = ps.neck + Rotate2D(float2(0.0, 0.22), ps.tilt * 0.5);

    float2 shoulderOff = Rotate2D(float2(0.11, -0.03), lean);
    ps.shoulderL = ps.neck - float2(shoulderOff.x, -shoulderOff.y);
    ps.shoulderR = ps.neck + shoulderOff;
    float2 dL = ArmDir(upL, -1.0);
    float2 dR = ArmDir(upR, 1.0);
    ps.elbowL = ps.shoulderL + dL * 0.17;
    ps.elbowR = ps.shoulderR + dR * 0.17;
    ps.handL = ps.elbowL + Rotate2D(dL, -foreL) * 0.16;
    ps.handR = ps.elbowR + Rotate2D(dR, foreR) * 0.16;

    ps.footL = float2(-0.09 - spread, jump);
    ps.footR = float2(0.09 + spread, jump);
    float kneeOut = 0.03 + dip * 0.05;
    ps.kneeL = (ps.hip + float2(-0.06, 0.0) + ps.footL) * 0.5 + float2(-kneeOut, 0.0);
    ps.kneeR = (ps.hip + float2(0.06, 0.0) + ps.footR) * 0.5 + float2(kneeOut, 0.0);
    return ps;
}

// One dancer in local units; returns color and coverage. Below the feet (p.y < 0)
// the same dancer is drawn mirrored and dimmed as a reflection in the wet stage.
float4 DrawDancer(float2 p, int slot, bool hasFace, int move, float3 color, float px, float lod)
{
    float reflection = 1.0;
    if (p.y < 0.0)
    {
        p.y = -p.y;
        p.x += sin(p.y * 40.0 + Time * 3.0) * 0.008;
        reflection = 0.28 * exp(-p.y * 2.0);
    }
    if (abs(p.x) > 0.6 || p.y > 1.35) return float4(0.0, 0.0, 0.0, 0.0);

    Pose ps = DancePose(move, CustomParam7, AudioKick);

    // Neon tube body
    float limbs = min(min(SegDist(p, ps.shoulderL, ps.elbowL), SegDist(p, ps.elbowL, ps.handL)),
                      min(SegDist(p, ps.shoulderR, ps.elbowR), SegDist(p, ps.elbowR, ps.handR)));
    limbs = min(limbs, min(min(SegDist(p, ps.hip + float2(-0.06, 0.0), ps.kneeL), SegDist(p, ps.kneeL, ps.footL)),
                           min(SegDist(p, ps.hip + float2(0.06, 0.0), ps.kneeR), SegDist(p, ps.kneeR, ps.footR))));
    limbs = min(limbs, SegDist(p, ps.shoulderL, ps.shoulderR));
    float torso = SegDist(p, ps.hip, ps.neck) - 0.065;
    float d = min(limbs - 0.032, torso);
    float core = 1.0 - smoothstep(0.0, px * 1.5, d);
    float halo = exp(-max(d, 0.0) * 28.0) * 0.55;
    float3 col = color * halo * (0.7 + AudioKick * 0.5) + lerp(color, float3(1.0, 1.0, 1.0), 0.55) * core;
    float alpha = max(core, halo * 0.6);

    // Head: the face inside a glowing ring that pulses on the beat
    float2 hq = Rotate2D(p - ps.head, -ps.tilt) / 0.2;
    float r = length(hq);
    if (r < 1.3)
    {
        float4 face = hasFace ? SampleFace(slot, hq, lod) : PlaceholderFace(hq, color);
        face.a *= step(r, 1.0);
        // Undo the final tone curve so the faces keep their natural colors
        float3 faceLinear = -log(max(1.0 - face.rgb * 0.97, 0.0001)) / 1.25;
        faceLinear += color * smoothstep(0.65, 1.0, r) * 0.25; // neon rim light
        col = lerp(col, faceLinear, face.a);
        alpha = max(alpha, face.a);

        float ring = exp(-Sq((r - 1.06) / 0.07)) * (0.7 + BeatPulse * 0.9 * SafetyDamping);
        col += color * ring * 1.2;
        alpha = max(alpha, ring * 0.8);
    }
    return float4(col * reflection, alpha * reflection);
}

// -----------------------------------------------------------------------------
// 3. Cyberpunk City
// -----------------------------------------------------------------------------
float3 SkylineLayer(float2 uv, float horizon, float colW, float minH, float rangeH, float scroll, float3 bodyCol, float windowGain, float seed)
{
    float x = uv.x + scroll;
    float cIdx = floor(x / colW);
    float h1 = Hash11(cIdx * 1.31 + seed);
    float top = horizon + minH + h1 * h1 * rangeH;
    if (uv.y > top) return float3(-1.0, -1.0, -1.0);

    float3 col = bodyCol;
    float2 wc = float2(x / (colW * 0.12), (uv.y - horizon) / (colW * 0.16));
    float2 wId = floor(wc);
    float2 wf = frac(wc);
    float inWin = step(0.25, wf.x) * step(wf.x, 0.75) * step(0.3, wf.y) * step(wf.y, 0.7);
    float lit = step(0.72, Hash21(wId + seed));
    float flicker = 0.75 + 0.25 * sin(Time * 3.0 + Hash21(wId) * 30.0) + AudioTreble * 0.4;
    float3 winCol = lerp(float3(0.2, 0.9, 1.0), float3(1.0, 0.25, 0.8), step(0.5, Hash21(wId * 1.7)));
    winCol = lerp(winCol, float3(1.0, 0.75, 0.3), step(0.85, Hash21(wId * 2.3)));
    col += winCol * inWin * lit * flicker * windowGain;

    // Vertical neon sign on some buildings, blinking red beacon on the roof
    float inCol = frac(x / colW);
    if (h1 > 0.55)
    {
        float signGlow = (1.0 - smoothstep(0.0, 0.08, abs(inCol - 0.3))) * step(top - rangeH * 0.35, uv.y) * step(uv.y, top - 0.02);
        col += DancerColor(int(fmod(cIdx, 6.0))) * signGlow * (0.8 + 0.4 * sin(Time * 7.0 + cIdx));
    }
    float beacon = exp(-length(float2((inCol - 0.5) * colW, uv.y - top - 0.008)) * 300.0) * step(0.5, frac(Time * 0.8 + h1));
    col += float3(1.0, 0.1, 0.1) * beacon;
    return col;
}

// Billboard showing the live spectrum
float3 EqBillboard(float2 uv, float2 center, float2 halfSize, float3 tint)
{
    float2 q = (uv - center) / halfSize;
    if (abs(q.x) > 1.0 || abs(q.y) > 1.0) return float3(0.0, 0.0, 0.0);
    float bars = 16.0;
    float bi = floor((q.x * 0.5 + 0.5) * bars);
    float level = GetAudioFFT(0.02 + bi / bars * 0.5) * 1.2;
    float barMask = step(abs(frac((q.x * 0.5 + 0.5) * bars) - 0.5), 0.35);
    float lit = step(q.y * 0.5 + 0.5, level) * barMask;
    float frame = step(0.94, max(abs(q.x), abs(q.y)));
    return tint * (lit * 1.3 + frame * 0.8 + 0.06);
}

float3 CyberCity(float2 uv, float horizon)
{
    // Smoggy sky: deep indigo overhead, magenta haze toward the horizon
    float3 col = lerp(float3(0.22, 0.03, 0.22), float3(0.015, 0.01, 0.05), saturate((uv.y - horizon) * 1.4));

    // Flying-car light trails crossing the sky
    for (int c = 0; c < 4; ++c)
    {
        float lane = horizon + 0.35 + float(c) * 0.13;
        float speed = 0.08 + Hash11(float(c) * 3.3) * 0.12;
        float dir = (fmod(float(c), 2.0) < 1.0) ? 1.0 : -1.0;
        float head = (frac(Time * speed + Hash11(float(c) * 7.1)) * 4.4 - 2.2) * dir;
        float along = (uv.x - head) * dir;
        float trail = exp(-Sq((uv.y - lane) / 0.004)) * smoothstep(-0.25, 0.0, along) * (1.0 - smoothstep(0.0, 0.01, along));
        col += ((dir > 0.0) ? float3(1.0, 0.3, 0.3) : float3(0.9, 0.95, 1.0)) * trail * 1.2;
    }

    // Far and near skyline layers drifting at different speeds
    float3 far = SkylineLayer(uv, horizon, 0.07, 0.12, 0.5, Time * 0.006, float3(0.05, 0.03, 0.11), 0.45, 3.0);
    if (far.x >= 0.0) col = lerp(col, far, 0.85);
    float3 nearL = SkylineLayer(uv, horizon, 0.16, 0.25, 0.85, Time * 0.015, float3(0.012, 0.01, 0.03), 0.8, 11.0);
    if (nearL.x >= 0.0) col = nearL;

    // Equalizer billboards on the skyline
    col += EqBillboard(uv, float2(-1.1 + 0.05 * sin(Time * 0.1), horizon + 0.62), float2(0.22, 0.1), float3(0.1, 0.9, 1.0));
    col += EqBillboard(uv, float2(1.05, horizon + 0.78), float2(0.18, 0.09), float3(1.0, 0.25, 0.8));

    // Haze along the horizon
    col += float3(0.4, 0.08, 0.35) * exp(-abs(uv.y - horizon) * 8.0) * 0.35 * (1.0 + AudioSubBass * 0.4);
    return col;
}

// -----------------------------------------------------------------------------
// 4. Main Pixel Shader
// -----------------------------------------------------------------------------
float4 PSMain(VSOutput input) : SV_Target
{
    float2 uv = input.UV * 2.0 - 1.0;
    uv.x *= AspectRatio;
    float px = 2.0 / Resolution.y;
    float horizon = -0.15;
    float beat = CustomParam7;

    // Wet neon stage floor: perspective grid (evaluated up front so fwidth stays in uniform control flow)
    float depth = 0.45 / max(horizon - uv.y, 0.02);
    float2 gridCoord = float2(uv.x * depth * 1.6, depth + Time * 0.6);
    float2 gfw = max(fwidth(gridCoord), float2(0.0001, 0.0001));
    float2 gd = abs(frac(gridCoord - 0.5) - 0.5) / gfw;
    float gridLine = (1.0 - min(min(gd.x, gd.y), 1.0)) * saturate(1.0 - max(gfw.x, gfw.y) * 0.8);

    float3 col;
    if (uv.y > horizon)
    {
        col = CyberCity(uv, horizon);
    }
    else
    {
        // Reflection of the city in the wet floor, plus the pulsing grid
        float2 ruv = float2(uv.x + sin(uv.y * 60.0 + Time * 2.0) * 0.004, 2.0 * horizon - uv.y);
        col = CyberCity(ruv, horizon) * 0.3 * exp(-(horizon - uv.y) * 1.5);
        float3 gridCol = lerp(float3(1.0, 0.2, 0.8), float3(0.1, 0.9, 1.0), 0.5 + 0.5 * sin(gridCoord.x * 0.5));
        col += gridCol * gridLine * 0.5 * (0.6 + AudioSubBass * 0.7) * exp(-(horizon - uv.y) * 0.6);
    }

    // Beat lasers fanning down from the top corners
    for (int l = 0; l < 2; ++l)
    {
        float side = (l == 0) ? -1.0 : 1.0;
        float2 origin = float2(side * 1.9, 1.05);
        float ang = -1.5708 - side * (0.45 + 0.25 * sin(beat * 0.785 + float(l)));
        float2 dir = float2(cos(ang), sin(ang));
        float2 rel = uv - origin;
        float along = dot(rel, dir);
        float dist = abs(rel.x * dir.y - rel.y * dir.x);
        float beam = exp(-dist / 0.004) * step(0.0, along) * (0.25 + BeatPulse * 0.9 * SafetyDamping);
        col += ((l == 0) ? float3(0.1, 0.9, 1.0) : float3(1.0, 0.2, 0.8)) * beam * 0.8;
    }

    // Dancers: count the faces in the atlas (placeholder dancers when there are none)
    int faceSlots[6];
    int count = 0;
    for (int s = 0; s < 6; ++s)
    {
        if (FacePresent(s))
        {
            faceSlots[count] = s;
            count += 1;
        }
    }
    bool haveFaces = count > 0;
    if (!haveFaces)
    {
        for (int s = 0; s < 4; ++s) faceSlots[s] = s;
        count = 4;
    }

    // Two rows; every 32 beats everyone slides one spot along so each dancer gets a turn up front
    int frontCount = (count + 1) / 2;
    int backCount = count - frontCount;
    float block = floor(beat / 32.0);
    float slide = smoothstep(30.0, 32.0, fmod(beat, 32.0));
    bool syncMoves = fmod(floor(beat / 16.0), 2.0) < 1.0;

    float4 back = float4(0.0, 0.0, 0.0, 0.0);
    float4 front = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < 6; ++i)
    {
        if (i >= count) break;
        float spotA = fmod(float(i) + block, float(count));
        float spotB = fmod(float(i) + block + 1.0, float(count));

        // Spots form a loop around the stage: front row left to right, then back row right to left,
        // so every move is to a neighboring spot and nobody crosses paths
        float3 placeA, placeB;
        for (int k = 0; k < 2; ++k)
        {
            float spot = (k == 0) ? spotA : spotB;
            bool isFront = spot < float(frontCount);
            float rowIdx = isFront ? spot : float(count - 1) - spot;
            float rowN = isFront ? float(frontCount) : float(max(backCount, 1));
            float span = isFront ? 1.2 : 0.95;
            float3 place = float3(lerp(-span, span, (rowIdx + 0.5) / rowN), isFront ? -0.95 : -0.58, isFront ? 0.74 : 0.56);
            if (k == 0) placeA = place;
            else placeB = place;
        }
        float3 place = lerp(placeA, placeB, slide);

        float2 lp = (uv - place.xy) / place.z;
        int slot = faceSlots[i];
        int move = int(fmod(floor(beat / 8.0) + (syncMoves ? 0.0 : float(slot)), 4.0) + 0.5);
        float lod = log2(max(488.0 / (0.4 * place.z * Resolution.y * 0.5), 1.0));
        float4 dancer = DrawDancer(lp, slot, haveFaces, move, DancerColor(slot), px / place.z, lod);

        // Spotlight cone from above
        float cone = exp(-Sq((uv.x - place.x) / (0.12 + (1.0 - uv.y) * 0.08))) * smoothstep(place.y - 0.1, 1.0, uv.y);
        col += DancerColor(slot) * cone * 0.05 * (0.7 + AudioKick * 0.6);

        if (place.y > -0.77) back = float4(lerp(back.rgb, dancer.rgb, dancer.a), max(back.a, dancer.a));
        else front = float4(lerp(front.rgb, dancer.rgb, dancer.a), max(front.a, dancer.a));
    }
    col = lerp(col, back.rgb, back.a * 0.9);
    col = lerp(col, front.rgb, front.a);

    // Rain over everything
    float2 rq = Rotate2D(uv, 0.22) * float2(70.0, 3.0) + float2(0.0, Time * (6.0 + AudioKick * 2.0));
    float2 rId = floor(rq);
    float drop = step(0.85, Hash21(rId)) * (1.0 - smoothstep(0.0, 0.5, abs(frac(rq.y) - Hash11(rId.x) * 0.5 - 0.25)));
    col += float3(0.5, 0.6, 0.9) * drop * (1.0 - smoothstep(0.0, 0.15, abs(frac(rq.x) - 0.5))) * 0.12;

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
