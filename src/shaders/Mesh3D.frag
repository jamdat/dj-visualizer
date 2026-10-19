#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vBarycentric;

out vec4 outFragColor;

layout(std140) uniform AudioVisualizerParams {
    // Vector 1: General Time & Screen
    float Time;
    float DeltaTime;
    vec2 Resolution;

    // Vector 2: Frequency Bands
    float AudioSubBass;
    float AudioKick;
    float AudioLowMid;
    float AudioHighMid;

    // Vector 3: Dynamics & Energy
    float AudioTreble;
    float BeatPulse;
    float DropIntensity;
    float EnergyAccumulator;

    // Vector 4: UI Custom Parameters 0..3
    float CustomParam0; // Zoom / Camera distance
    float CustomParam1; // Speed / Complexity
    float CustomParam2; // Logo Scale / Grid Scale
    float CustomParam3; // Glow / Distortion

    // Vector 5: UI Custom Parameters 4..7
    float CustomParam4; // Logo Y Position Offset
    float CustomParam5; // 3D Mesh Toggle (1.0 = 3D Mesh)
    float CustomParam6; // Yaw Rotation Angle
    float CustomParam7; // Reserved

    // Vector 6: Primary Palette Color
    vec4 ColorPrimary;

    // Vector 7: Secondary Palette Color
    vec4 ColorSecondary;

    // Vector 8: Accent Palette Color
    vec4 ColorAccent;

    // Vector 9: Global Render FX & Photosafety
    float BloomIntensity;
    float GlitchRate;
    float WireframeDensity;
    float MasterBrightness;

    // Vector 10: Dad Punk Primary Layers
    float PyramidWeight;
    float LogoWeight;
    float SpookyWeight;
    float TrailsWeight;

    // Vector 11: Modes & Dynamic Controls
    int PyramidPattern;
    int LogoStyle;
    float SafetyDamping;
    float ParticleIntensity;

    // Vector 12: Environment & Lasers
    float LaserIntensity;
    float SkyLayerWeight;
    float FloorLayerWeight;
    float Bpm;

    // Vector 13: Additional Scene Weights & Screen
    float AspectRatio;
    float HorizonWeight;
    float LedWallWeight;
    float ShapeCrossfade;

    // Vector 14..17: Custom Text ASCII
    uvec4 CustomText0;
    uvec4 CustomText1;
    uvec4 CustomText2;
    uvec4 CustomText3;
};

uniform int u_Style; // 0=Discovery Liquid Chrome, 1=Faceted 3D Crystal Mesh, 2=Neon Outline
uniform vec3 u_CameraPos;

vec3 HSVtoRGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(u_CameraPos - vWorldPos);
    vec3 R = reflect(-V, N);

    // Single-pass barycentric wireframe edge calculation
    vec3 d = fwidth(vBarycentric);
    vec3 a3 = smoothstep(vec3(0.0), d * 1.5, vBarycentric);
    float edgeFactor = min(min(a3.x, a3.y), a3.z);
    float wireframe = 1.0 - edgeFactor;
    // Flat cap faces are earcut-triangulated: hide those edges so letters read as solid shapes
    wireframe *= 1.0 - smoothstep(0.85, 0.98, abs(N.z));

    vec3 col = vec3(0.0);

    if (u_Style == 0) // 0: Discovery 2001 Liquid Chrome (Reference Image 1)
    {
        // High-contrast liquid chrome horizon & sky reflections
        float chromeHorizon = smoothstep(-0.06, 0.06, R.y);
        vec3 skyReflect = mix(vec3(0.12, 0.16, 0.28), vec3(0.95, 0.98, 1.0), pow(max(R.y, 0.0), 0.35));
        vec3 groundReflect = mix(vec3(0.02, 0.02, 0.04), vec3(0.40, 0.40, 0.45), pow(max(-R.y, 0.0), 0.5));
        vec3 chromeBody = mix(groundReflect, skyReflect, chromeHorizon);

        // Specular lights
        vec3 lightDir1 = normalize(vec3(0.35, 0.75, 0.5));
        float spec1 = pow(max(dot(R, lightDir1), 0.0), 38.0) * (3.0 + AudioKick * 1.5);
        vec3 lightDir2 = normalize(vec3(-0.4, 0.35, 0.6));
        float spec2 = pow(max(dot(R, lightDir2), 0.0), 16.0) * 1.2;

        // Prismatic rainbow caustics along bevel edge normals
        float angle = atan(N.y, N.x);
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        float rainbowPhase = fract(angle / 6.28318 + fresnel * 0.8 - EnergyAccumulator * 0.15);
        vec3 rainbow = HSVtoRGB(vec3(rainbowPhase, 0.95, 1.0));

        col = chromeBody + vec3(spec1) + vec3(spec2) * ColorPrimary.rgb + rainbow * fresnel * (2.2 + AudioKick * 1.5);
    }
    else if (u_Style == 1) // 1: Faceted 3D Crystal Mesh (Reference Image 2)
    {
        // Glowing wireframe triangle edges
        vec3 litWire = ColorAccent.rgb * (wireframe * 3.5 * (1.0 + AudioTreble * 1.5));

        // Facet sparkling specular reflection
        float facetLight = max(dot(N, normalize(vec3(0.3, 0.8, 0.5))), 0.0);
        vec3 crystalBody = mix(vec3(0.015, 0.02, 0.04), ColorPrimary.rgb * 0.45, facetLight);
        crystalBody += ColorSecondary.rgb * max(dot(N, normalize(vec3(-0.4, 0.3, 0.6))), 0.0) * 0.8;

        col = crystalBody + litWire;
    }
    else if (u_Style == 2) // 2: Neon Outline / Cyber Vector
    {
        float pulse = 1.0 + AudioKick * 0.8 + BeatPulse * 0.5;
        col = ColorAccent.rgb * (wireframe * 4.0 + 0.05) * pulse;
    }
    else if (u_Style == 3) // 3: Digital Scanlines / Cyber Slices Passing Through
    {
        float scanline = sin((vWorldPos.y + vWorldPos.x * 0.25) * 45.0 - Time * 12.0) * 0.5 + 0.5;
        scanline = pow(scanline, 4.0);
        float pulse = 1.0 + AudioKick * 1.2;
        vec3 wire = ColorAccent.rgb * wireframe * 2.5;
        vec3 slice = ColorPrimary.rgb * scanline * 4.0 * pulse;
        col = wire + slice + ColorSecondary.rgb * 0.1;
    }
    else // 4: Solid Daft Red / Gold Punk Finish
    {
        float diff = max(dot(N, normalize(vec3(0.2, 0.6, 0.8))), 0.0) * 0.8 + 0.2;
        vec3 base = ColorPrimary.rgb * diff * (1.0 + AudioKick * 0.3);
        float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5);
        col = base + ColorAccent.rgb * rim * 2.0 + ColorAccent.rgb * (wireframe * 1.5);
    }

    // Vignette pulse and edge darkening over 3D Hero Mesh (Foremost Overlay)
    if (SpookyWeight > 0.01) {
        vec2 screenUV = gl_FragCoord.xy / max(Resolution, vec2(1.0, 1.0));
        float edgeDist = length(screenUV - 0.5) * 2.0;
        float darkVignette = clamp(1.0 - smoothstep(0.7, 1.4, edgeDist) * 0.45 * SpookyWeight, 0.0, 1.0);
        col *= darkVignette;

        if (DropIntensity > 0.1 || BeatPulse > 0.7) {
            float dropBorder = smoothstep(0.7, 1.4, edgeDist) * (DropIntensity * 0.3 + BeatPulse * 0.2);
            col += ColorAccent.rgb * dropBorder * 0.35 * SafetyDamping * SpookyWeight;
        }
    }

    col *= MasterBrightness;
    outFragColor = vec4(col, 1.0);
}
