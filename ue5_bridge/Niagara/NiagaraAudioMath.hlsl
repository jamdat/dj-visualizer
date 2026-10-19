// =============================================================================
// NiagaraAudioMath.hlsl - Audio-Reactive Niagara GPU Particle Math
// =============================================================================

#pragma once

// 1. Audio-Reactive Vortex Velocity Field
float3 AudioVortexField(float3 particlePos, float audioKick, float audioSubBass, float time)
{
    float2 rad = particlePos.xz;
    float dist = length(rad);
    if (dist < 0.001) return float3(0, 0, 0);

    float2 tangent = float2(-rad.y, rad.x) / dist;
    float spinSpeed = (1.5 + audioSubBass * 3.0);

    float3 vel;
    vel.x = tangent.x * spinSpeed;
    vel.y = sin(dist * 2.0 - time * 3.0) * (0.5 + audioKick * 2.0);
    vel.z = tangent.y * spinSpeed;

    return vel;
}

// 2. Audio-Reactive Particle Ribbon Wave
float3 AudioRibbonDisplacement(float ribbonNormPos, float audioLowMid, float audioHighMid, float time)
{
    float freq = ribbonNormPos * 12.0 - time * 4.0;
    float amp = 50.0 * (0.2 + audioLowMid * 1.5);

    float3 disp;
    disp.x = sin(freq) * amp;
    disp.y = cos(freq * 1.5) * amp * (0.5 + audioHighMid);
    disp.z = 0.0;

    return disp;
}
