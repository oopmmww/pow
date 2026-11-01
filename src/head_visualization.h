#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "assets/neutral_head_resource.h"

// Embedded reference-style head image with an independently animated AREA overlay.
static void DrawHeadPreview(ImDrawList *dl, ImVec2 lo, ImVec2 hi, float area, float sizeValue, int view,
                            float displayScale = 1.f)
{
    const float pi = 3.14159265f;
    const float w = hi.x - lo.x, h = hi.y - lo.y;
    static float zoom = 1.f, shownArea = .15f, shownSize = .45f;
    if (ImGui::IsMouseHoveringRect(lo, hi))
        zoom = ImClamp(zoom + ImGui::GetIO().MouseWheel * .08f, .65f, 1.35f);
    shownArea += (area - shownArea) * ImMin(1.f, ImGui::GetIO().DeltaTime * 12.f);
    shownSize += (sizeValue - shownSize) * ImMin(1.f, ImGui::GetIO().DeltaTime * 12.f);
    // Visual angle only: open a symmetric sector around the upward direction.
    const float sizeProgress = ImClamp((shownSize - .10f) / 1.12f, 0.f, 1.f);
    const float halfAngle = (.025f + .975f * sizeProgress) * pi;

    ImVec2 c(lo.x + w * .5f, lo.y + h * .51f);
    float s = ImMin(w * .21f, h * .23f) * zoom * displayScale;
    dl->PushClipRect(lo, hi, true);

    // Match the visible skull, not the transparent padding of its PNG.
    // UI level 1 hugs the head; every higher level expands the preview.
    // This is an illustration, not a measured in-game radius.
    const float expansion = ImClamp((shownArea - .01f) / .79f, 0.f, 1.f) * s * .585f;
    const float rx = s * .70f + expansion;
    const float ry = s * .97f + expansion;
    const ImVec2 areaCenter(c.x, c.y - s * .03f);
    auto point = [&](float a, float k = 1.f)
    {
        const float taper = 1.f - .08f * sinf(a);
        return ImVec2(areaCenter.x + cosf(a) * rx * taper * k, areaCenter.y + sinf(a) * ry * k);
    };
    auto ring = [&](float k, ImU32 col, float thickness)
    {
        for (int i = 0; i <= 128; i++)
            dl->PathLineTo(point(i * 2 * pi / 128, k));
        dl->PathStroke(col, ImDrawFlags_Closed, thickness);
    };
    for (int i = 0; i < 128; i++)
        dl->PathLineTo(point(i * 2 * pi / 128));
    dl->PathFillConvex(IM_COL32(24, 235, 117, 17));
    for (int i = 5; i >= 1; i--)
        ring(1.f, IM_COL32(24, 235, 117, 8), float(i * 3));
    ring(1.f, IM_COL32(76, 255, 151, 210), 1.4f);
    for (int i = 0; i < 64; i += 2)
        dl->AddLine(point(i * 2 * pi / 64, 1.06f), point((i + 1) * 2 * pi / 64, 1.06f),
                    IM_COL32(45, 235, 135, 130));
    float time = float(ImGui::GetTime());
    float pulse = fmodf(time * .3f, 1.f);
    ring(1.06f + pulse * .14f, IM_COL32(45, 235, 135, int((1 - pulse) * 28)), 1.f);
    for (int i = 0; i < 25; i++)
    {
        float a = time * .65f - i * .012f;
        dl->AddLine(point(a), point(a - .012f), IM_COL32(150, 255, 200, 255 - i * 9), 2.5f);
    }
    for (int i = 0; i < 4; i++)
        dl->AddCircleFilled(point(i * pi / 2), 4.f, IM_COL32(161, 255, 200, 255));
    static ImTextureID headTexture{};
    static bool textureAttempted = false;
    if (!textureAttempted && var->dx.device)
    {
        widgets->load_texture_from_memory(head_reference_png, sizeof(head_reference_png), &headTexture);
        textureAttempted = true;
    }
    if (headTexture)
    {
        // Fixed reference-style render, with alpha; AREA stays a separate animated layer.
        float half = s * 1.22f;
        const float halfWidth = half * (1322.f / 1190.f);
        const ImVec2 imageMin(c.x - halfWidth, c.y - half), imageMax(c.x + halfWidth, c.y + half);
        dl->AddImage(headTexture, imageMin, imageMax);
        // A true fan: fixed center and radius, Size controls only its angle.
        // Triangles work for narrow sectors and sectors wider than 180 degrees.
        const ImVec2 pivot(c.x, c.y - s * .03f);
        const float skullX = s * .69f, skullY = s * .96f;
        const int segments = ImMax(8, int(halfAngle * 64.f));
        const float startAngle = -pi * .5f - halfAngle;
        auto fanPoint = [&](float angle)
        {
            return ImVec2(pivot.x + cosf(angle) * skullX * (1.f - .08f * sinf(angle)),
                          pivot.y + sinf(angle) * skullY);
        };
        const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
        const float uiScale = ImClamp(s / 100.f, .65f, 1.6f);
        auto radialPoint = [&](float angle, float radius)
        {
            ImVec2 edge = fanPoint(angle);
            return ImVec2(pivot.x + (edge.x - pivot.x) * radius, pivot.y + (edge.y - pivot.y) * radius);
        };
        // Soft radial tint: transparent near the pivot, richer at the rim.
        // Shared vertices avoid dark overlaps between slices.
        auto tint = [](float r)
        {
            float eased = r * r * (3.f - 2.f * r);
            return IM_COL32(38, 232, 138, int(12.f + 76.f * eased));
        };
        auto triangle = [&](ImVec2 a, ImVec2 b, ImVec2 d, ImU32 ca, ImU32 cb, ImU32 cd)
        {
            dl->PrimReserve(3, 3);
            unsigned int base = dl->_VtxCurrentIdx;
            dl->PrimWriteIdx((ImDrawIdx)base);
            dl->PrimWriteIdx((ImDrawIdx)(base + 1));
            dl->PrimWriteIdx((ImDrawIdx)(base + 2));
            dl->PrimWriteVtx(a, uv, ca);
            dl->PrimWriteVtx(b, uv, cb);
            dl->PrimWriteVtx(d, uv, cd);
        };
        for (int band = 0; band < 12; ++band)
        {
            float inner = band / 12.f, outer = (band + 1) / 12.f;
            for (int i = 0; i < segments; ++i)
            {
                float a = startAngle + 2.f * halfAngle * i / segments;
                float b = startAngle + 2.f * halfAngle * (i + 1) / segments;
                ImVec2 p0 = radialPoint(a, inner), p1 = radialPoint(a, outer);
                ImVec2 p2 = radialPoint(b, outer), p3 = radialPoint(b, inner);
                triangle(p0, p1, p2, tint(inner), tint(outer), tint(outer));
                if (band > 0)
                    triangle(p0, p2, p3, tint(inner), tint(outer), tint(inner));
            }
        }
        auto arc = [&](float radius, ImU32 color, float thickness)
        {
            for (int i = 0; i <= segments; ++i)
                dl->PathLineTo(radialPoint(startAngle + 2.f * halfAngle * i / segments, radius));
            dl->PathStroke(color, 0, thickness * uiScale);
        };
        arc(1.f, IM_COL32(35, 245, 140, 12), 9.f);
        arc(1.f, IM_COL32(50, 255, 150, 24), 5.f);
        arc(1.f, IM_COL32(116, 255, 182, 230), 1.5f);
        // An inset hairline gives the selected sector depth without a busy grid.
        arc(.94f, IM_COL32(112, 255, 180, 44), .7f);
        if (sizeProgress < .999f)
        {
            for (float angle : {startAngle, startAngle + 2.f * halfAngle})
            {
                // Fade the spokes near the center to keep the head readable.
                for (int i = 0; i < 16; ++i)
                {
                    float r0 = .08f + i * .92f / 16, r1 = .08f + (i + 1) * .92f / 16;
                    dl->AddLine(radialPoint(angle, r0), radialPoint(angle, r1),
                                IM_COL32(110, 248, 175, int(20 + 110 * r1)), uiScale);
                }
                ImVec2 endpoint = fanPoint(angle);
                dl->AddCircleFilled(endpoint, 7.f * uiScale, IM_COL32(50, 245, 140, 22), 24);
                dl->AddCircleFilled(endpoint, 4.3f * uiScale, IM_COL32(14, 34, 26, 255), 24);
                dl->AddCircle(endpoint, 4.3f * uiScale, IM_COL32(125, 255, 185, 230), 24, uiScale);
                dl->AddCircleFilled(endpoint, 2.3f * uiScale, IM_COL32(225, 255, 240, 255), 24);
            }
        }
        dl->AddCircle(pivot, 3.4f * uiScale, IM_COL32(135, 235, 180, 100), 24, uiScale);
        dl->AddCircleFilled(pivot, 1.3f * uiScale, IM_COL32(190, 255, 220, 200), 16);
    }
    else
    {
        dl->AddText({c.x - 65, c.y}, IM_COL32(210, 215, 220, 255), "Head preview unavailable");
    }
    dl->AddText({c.x - 20, lo.y + 14}, IM_COL32(195, 210, 205, 255),
                view == 0   ? "FRONT"
                : view == 1 ? "FRONT VIEW"
                            : "SIDE VIEW");
    dl->AddText({lo.x + 12, c.y}, IM_COL32(50, 240, 150, 255), "L");
    dl->AddText({hi.x - 20, c.y}, IM_COL32(50, 240, 150, 255), "R");
    dl->PopClipRect();
}
