<!--
  Copyright (C) 2008 by Frank Richter

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
-->
<include>
<![CDATA[

#ifndef __LIGHTFUNCS_CG_INC__
#define __LIGHTFUNCS_CG_INC__

#define MAX_LIGHTS 8

half Attenuation_Linear (float d, float invLightRadius)
{
  return saturate (1 - d * invLightRadius);
}

half Attenuation_Inverse (float d)
{
  return 1/d;
}

half Attenuation_Realistic (float d)
{
  return 1/(d*d);
}

half Attenuation_CLQ (float d, float3 coeff)
{
  return 1/(dot (half3 (1, d, d*d), half3 (coeff)));
}

half Light_Spot (half3 surfToLight, half3 lightDir, half falloffInner, half falloffOuter)
{
  return smoothstep (falloffOuter, falloffInner, dot (surfToLight, lightDir));
}


struct LightProperties
{
  // Number of lights provided
  int count;
  // Light world-space position
  float4 posWorld[MAX_LIGHTS];
  // Transformation from light space to world space
  float4x4 transform[MAX_LIGHTS];
  // Transformation from world space to light space
  float4x4 transformInv[MAX_LIGHTS];
  // Diffuse color
  float3 colorDiffuse[MAX_LIGHTS];
  // Specular color
  float3 colorSpecular[MAX_LIGHTS];
  // Attenuation vector (XYZ are CLQ coefficients; W is light radius)
  float4 attenuationVec[MAX_LIGHTS];
  // Cosine of inner falloff angle
  float falloffInner[MAX_LIGHTS];
  // Cosine of outerr falloff angle
  float falloffOuter[MAX_LIGHTS];
};
LightProperties lightProps;

interface LightSpace
{
  half3 GetDirection();
  half3 GetSurfaceToLight();
  float GetLightDistance ();
};

struct LightSpaceWorld : LightSpace
{
  half3 dir;
  half3 surfToLight;
  float lightDist;

  void Init (int lightNum, float4 surfPositionWorld)
  {
    float4 pos = lightProps.posWorld[lightNum];
    float4x4 lightTransform = lightProps.transform[lightNum];
    float3 lightDirW = mul (lightTransform, float4 (0, 0, 1, 0)).xyz;
    dir = lightDirW;
    surfToLight = (pos - surfPositionWorld).xyz;
    lightDist = length (surfToLight);
    surfToLight = normalize (surfToLight);
  }
  half3 GetDirection() { return dir; }
  half3 GetSurfaceToLight() { return surfToLight; }
  float GetLightDistance () {return lightDist; }
};

struct LightSpaceTangent : LightSpace
{
  half3 dir;
  half3 surfToLight;
  float lightDist;
  
  void InitVP (int lightNum, 
               float4x4 world2tangent, float4 surfPositionWorld,
               out half3 vp_dir, out float3 vp_surfToLight)
  {
    float4 pos = lightProps.posWorld[lightNum];
    float4x4 lightTransform = lightProps.transform[lightNum];
    float4 lightDirW = mul (lightTransform, float4 (0, 0, 1, 0));
    vp_dir = mul (world2tangent, lightDirW).xyz;
    vp_surfToLight = mul (world2tangent, float4 ((pos - surfPositionWorld).xyz, 0)).xyz;
  }

  void Init (half3 vp_dir, float3 vp_surfToLight)
  {
    dir = vp_dir;
    lightDist = length (vp_surfToLight);
    surfToLight = normalize (vp_surfToLight);
  }
  half3 GetDirection() { return dir; }
  half3 GetSurfaceToLight() { return surfToLight; }
  float GetLightDistance () {return lightDist; }
};

interface Shadow
{
  half GetVisibility();
};

struct ShadowNone : Shadow
{
  half GetVisibility() { return 1; }
};

struct LightPropertiesShadowMap
{
  // Transformation from light to shadow map space
  float4x4 shadowMapTF[MAX_LIGHTS];
  // Shadow map
  sampler2D shadowMap[MAX_LIGHTS];
  // Shadow map pixel size + dimensions
  float4 shadowMapPixels[MAX_LIGHTS];
};
LightPropertiesShadowMap lightPropsSM;

struct ShadowShadowMapDepth : Shadow
{
  float4x4 shadowMapTF;
  float4 shadowMapCoords;
  float4 shadowMapCoordsProj;
  sampler2D shadowMap;
  float bias;
  float gradient;

  void InitVP (int lightNum, float4 surfPositionWorld,
               float3 normWorld,
               out float4 vp_shadowMapCoords,
               out float vp_gradientApprox)
  {
    float4x4 lightTransformInv = lightProps.transformInv[lightNum];
    // Transform world position into light space
    float4 view_pos = mul(lightTransformInv, surfPositionWorld);
    // Transform position in light space into "shadow map space"
    float4 shadowMapCoords;
    shadowMapTF = lightPropsSM.shadowMapTF[lightNum];
    /* CS' render-to-texture Y-flips render targets (so the upper left
       gets rendered to 0,0), we need to unflip here again. */
    float4x4 flipY;
    flipY[0] = float4 (1, 0, 0, 0);
    flipY[1] = float4 (0, -1, 0, 0);
    flipY[2] = float4 (0, 0, 1, 0);
    flipY[3] = float4 (0, 0, 0, 1);
    shadowMapTF = mul (shadowMapTF, flipY);
    shadowMapCoords = mul (shadowMapTF, view_pos);
    
    vp_shadowMapCoords = shadowMapCoords;
    
    float3 normL = mul(lightTransformInv, float4 (normWorld, 0)).xyz;
    float3 normShadow = normalize (mul (shadowMapTF, float4 (normL, 0)).xyz);
    vp_gradientApprox = 1 - saturate (dot (normShadow, float3 (0, 0, -1)));
  }
  
  void Init (int lightNum, float4 vp_shadowMapCoords, float vp_gradient)
  {
    shadowMapCoords = vp_shadowMapCoords;
    shadowMap = lightPropsSM.shadowMap[lightNum];
    gradient = vp_gradient;
    
    // Project SM coordinates
    shadowMapCoordsProj = shadowMapCoords;
    shadowMapCoordsProj.xyz /= shadowMapCoordsProj.w;
    shadowMapCoordsProj.xyz = (float3(0.5)*shadowMapCoordsProj.xyz) + float3(0.5);
    
    // @@@ FIXME: Needing such a high bias scale seems ridiculous!
    // Maybe fixing #471 would allow to reduce it.
    bias = (1.0/32768.0);
    bias *= 1 + (gradient*gradient*256);
  }
  
  half GetVisibility()
  {
    // Depth to compare against
    float compareDepth = shadowMapCoordsProj.z + bias;
    // Depth compare with shadow map texel
    half inLight = h4tex2D (shadowMap, float3 (shadowMapCoordsProj.xy, compareDepth)).x;
    return inLight;
  }
};

struct ShadowShadowMapVSM : Shadow
{
  float4x4 shadowMapTF;
  float4 shadowMapCoords;
  float4 shadowMapCoordsProj;
  sampler2D shadowMap;
  float bias;
  float gradient;

  void InitVP (int lightNum, float4 surfPositionWorld,
               float3 normWorld,
               out float4 vp_shadowMapCoords,
               out float vp_gradientApprox)
  {
    float4x4 lightTransformInv = lightProps.transformInv[lightNum];
    // Transform world position into light space
    float4 view_pos = mul(lightTransformInv, surfPositionWorld);
    // Transform position in light space into "shadow map space"
    float4 shadowMapCoords;
    shadowMapTF = lightPropsSM.shadowMapTF[lightNum];
    /* CS' render-to-texture Y-flips render targets (so the upper left
       gets rendered to 0,0), we need to unflip here again. */
    float4x4 flipY;
    flipY[0] = float4 (1, 0, 0, 0);
    flipY[1] = float4 (0, -1, 0, 0);
    flipY[2] = float4 (0, 0, 1, 0);
    flipY[3] = float4 (0, 0, 0, 1);
    shadowMapTF = mul (shadowMapTF, flipY);
    shadowMapCoords = mul (shadowMapTF, view_pos);
    
    vp_shadowMapCoords = shadowMapCoords;
    
    float3 normL = mul(lightTransformInv, float4 (normWorld, 0)).xyz;
    float3 normShadow = normalize (mul (shadowMapTF, float4 (normL, 0)).xyz);
    vp_gradientApprox = 1 - saturate (dot (normShadow, float3 (0, 0, -1)));
  }
  
  void Init (int lightNum, float4 vp_shadowMapCoords, float vp_gradient)
  {
    shadowMapCoords = vp_shadowMapCoords;
    shadowMap = lightPropsSM.shadowMap[lightNum];
    gradient = vp_gradient;
    
    // Project SM coordinates
    shadowMapCoordsProj = shadowMapCoords;
    shadowMapCoordsProj.xyz /= shadowMapCoordsProj.w;
    shadowMapCoordsProj.xyz = (float3(0.5)*shadowMapCoordsProj.xyz) + float3(0.5);
    
    bias = 0.000001;
    //bias *= 1 + (gradient*gradient*128);
  }
  
  half GetVisibility()
  {
    // Depth to compare against
    /* @@@ TODO: Should prolly do some scaling by light cutoff
       so that depth gets 1 at the cutoff */
    float compareDepth = (1-shadowMapCoordsProj.z) - bias;
    float2 shadowDepth = tex2D (shadowMap, shadowMapCoordsProj.xy).xy;
    if (compareDepth <= shadowDepth.x) return 1;
    float var = saturate (shadowDepth.y - shadowDepth.x*shadowDepth.x);
    float d = (shadowDepth.x-compareDepth);
    float p_max = var/(var+d*d);
    return p_max;
  }
};

interface Light
{
  half3 GetIncidence();
  half GetAttenuation();
};

struct LightDirectional : Light
{
  half3 dir;
  
  void Init (LightSpace space)
  {
    dir = -space.GetDirection();
  }
  half3 GetIncidence() { return dir; }
  half GetAttenuation() { return 1; }
};

struct LightPoint : Light
{
  half3 dir;
  
  void Init (LightSpace space)
  {
    dir = space.GetSurfaceToLight();
  }
  half3 GetIncidence() { return dir; }
  half GetAttenuation() { return 1; }
};

struct LightSpot : Light
{
  half3 dir;
  half spot;
  
  void Init (LightSpace space, half3 normal, half falloffInner, half falloffOuter)
  {
    dir = -space.GetDirection();
    spot = Light_Spot (space.GetSurfaceToLight(), dir, falloffInner, falloffOuter);
  }
  half3 GetIncidence() { return dir; }
  half GetAttenuation() { return spot; }
};
]]>

Light GetCurrentLight (LightSpace lightSpace, int lightNum, half3 surfNormal)
{
<?if vars."light type".int == consts.CS_LIGHT_DIRECTIONAL ?>
  LightDirectional ld;
  ld.Init (lightSpace);
  return ld;
<?elsif vars."light type".int == consts.CS_LIGHT_SPOTLIGHT ?>
  LightSpot ls;
  ls.Init (lightSpace, surfNormal, 
    lightProps.falloffInner[lightNum],
    lightProps.falloffOuter[lightNum]);
  return ls;
<?else?>
<?! Assume point light ?>
  LightPoint lp;
  lp.Init (lightSpace);
  return lp;
<?endif?>
}

<![CDATA[
void ComputeLight (LightSpace lightSpace, Light light, 
                   half3 eyeToSurf, half3 surfNormal,
                   half surfShininess, 
                   float3 lightDiffuse, float3 lightSpecular,
                   float4 lightAttenuationVec,
                   Shadow shadow, out float3 d, out float3 s)
{
  half3 L = light.GetIncidence();
  half3 H = normalize (lightSpace.GetSurfaceToLight() - eyeToSurf);
  half spot = light.GetAttenuation();
  
  float4 lightCoeff = lit (dot (surfNormal, L), dot (surfNormal, H),
    surfShininess);
  
  float lightDist = lightSpace.GetLightDistance();
  half attn;
  float attnRadius = lightAttenuationVec.w;
  if (attnRadius > 0)
    attn = Attenuation_Linear (lightDist, 1 / attnRadius);
  else
    attn = Attenuation_CLQ (lightDist, lightAttenuationVec.xyz);
  
  half shadowed = shadow.GetVisibility();
  attn *= shadowed;

  d = lightDiffuse * lightCoeff.y * spot * attn;
  s = lightSpecular * lightCoeff.z * spot * attn;
}

#endif // __LIGHTFUNCS_CG_INC__
 
]]>
</include>
