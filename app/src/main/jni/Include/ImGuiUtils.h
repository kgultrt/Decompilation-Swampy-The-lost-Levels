#pragma once

#include <cmath>     // for floor, fabsf, ceilf
#include <cstring>   // for memset
#include <cassert>   // for assert

namespace ImGui
{

    // ----------------------------------------------------------------------
    // 样式设置函数：快速切换亮色/暗色主题，并调整透明度
    // ----------------------------------------------------------------------
    inline void SetupImGuiStyle(bool bStyleDark_, float alpha_)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // 亮色风格（来自 Pacôme Danhiez）
        style.Alpha = 1.0f;
        style.FrameRounding = 3.0f;
        style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
        style.Colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // 原 ImGuiCol_ChildWindowBg
        style.Colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
        // ImGuiCol_ComboBg 已移除，使用 PopupBg 代替（已设置）
        style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        // 以下列颜色在新版中已由 Table 颜色替代，此处暂不设置（或可选用 ImGuiCol_Table... 系列）
        // style.Colors[ImGuiCol_Column]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        // style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
        // style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        // 关闭按钮颜色已合并到普通按钮，此处不再单独设置
        // style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
        // style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
        // style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f); // 原 ImGuiCol_ModalWindowDarkening

        if (bStyleDark_)
        {
            for (int i = 0; i < ImGuiCol_COUNT; i++) // 注意：通常使用 < ImGuiCol_COUNT，原代码使用 <= 但 ImGuiCol_COUNT 是最后一个枚举值，用 < 更安全
            {
                ImVec4& col = style.Colors[i];
                float H, S, V;
                ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

                if (S < 0.1f)
                {
                    V = 1.0f - V;
                }
                ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
                if (col.w < 1.00f)
                {
                    col.w *= alpha_;
                }
            }
        }
        else
        {
            for (int i = 0; i < ImGuiCol_COUNT; i++)
            {
                ImVec4& col = style.Colors[i];
                if (col.w < 1.00f)
                {
                    col.x *= alpha_;
                    col.y *= alpha_;
                    col.z *= alpha_;
                    col.w *= alpha_;
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    // 复选框辅助函数（基于纯文本）
    // ----------------------------------------------------------------------
    inline bool CheckBoxFont(const char* name_, bool* pB_, const char* pOn_ = "[X]", const char* pOff_ = "[  ]")
    {
        if (*pB_)
        {
            ImGui::Text(pOn_);
        }
        else
        {
            ImGui::Text(pOff_);
        }
        bool bHover = false;
        bHover = bHover || ImGui::IsItemHovered();
        ImGui::SameLine();
        ImGui::Text(name_);
        bHover = bHover || ImGui::IsItemHovered();
        if (bHover && ImGui::IsMouseClicked(0))
        {
            *pB_ = !*pB_;
            return true;
        }
        return false;
    }

    inline bool CheckBoxTick(const char* name_, bool* pB_)
    {
        return CheckBoxFont(name_, pB_);   // 使用默认的 "[X]" / "[  ]"
    }

    inline bool MenuItemCheckbox(const char* name_, bool* pB_)
    {
        bool retval = ImGui::MenuItem(name_);
        ImGui::SameLine();
        if (*pB_)
        {
            ImGui::Text("[X]");
        }
        else
        {
            ImGui::Text("[ ]");
        }
        if (retval)
        {
            *pB_ = !*pB_;
        }
        return retval;
    }

    // ----------------------------------------------------------------------
    // 帧时间直方图类
    // ----------------------------------------------------------------------
    struct FrameTimeHistogram
    {
        // 配置参数
        static const int NUM = 101;           // 直方柱数量（最后一个代表 >= (NUM-1)*dT 的时间）
        float  dT      = 0.001f;               // 每个直方柱宽度（秒），默认 1ms
        float  refresh = 1.0f / 60.0f;         // 目标刷新率对应的帧时间（如 60 FPS = 1/60 秒）

        static const int NUM_MARKERS = 2;
        float  markers[NUM_MARKERS] = { 0.99f, 0.999f };   // 百分位标记线

        // 数据
        ImVec2 size        = ImVec2(3.0f * NUM, 40.0f);    // 绘图区域大小
        float  lastdT      = 0.0f;
        float  timesTotal;
        float  countsTotal;
        float  times[NUM];
        float  counts[NUM];
        float  hitchTimes[NUM];
        float  hitchCounts[NUM];

        FrameTimeHistogram()
        {
            Clear();
        }

        void Clear()
        {
            timesTotal  = 0.0f;
            countsTotal = 0.0f;
            memset(times,       0, sizeof(times));
            memset(counts,      0, sizeof(counts));
            memset(hitchTimes,  0, sizeof(hitchTimes));
            memset(hitchCounts, 0, sizeof(hitchCounts));
        }

        int GetBin(float time_) const
        {
            int bin = (int)floor(time_ / dT);
            if (bin >= NUM)
                bin = NUM - 1;
            return bin;
        }

        void Update(float deltaT_)
        {
            if (deltaT_ < 0.0f)
            {
                assert(false);
                return;
            }
            int bin = GetBin(deltaT_);
            times[bin] += deltaT_;
            timesTotal  += deltaT_;
            counts[bin] += 1.0f;
            countsTotal += 1.0f;

            float hitch = fabsf(lastdT - deltaT_);
            int deltaBin = GetBin(hitch);
            hitchTimes[deltaBin] += hitch;
            hitchCounts[deltaBin] += 1.0f;
            lastdT = deltaT_;
        }

        void PlotRefreshLines(float total_ = 0.0f, float* pValues_ = nullptr) const
        {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 pad = style.FramePadding;
            ImVec2 min = ImGui::GetItemRectMin();
            min.x += pad.x;
            ImVec2 max = ImGui::GetItemRectMax();
            max.x -= pad.x;

            float xRefresh = (max.x - min.x) * refresh / (dT * NUM);
            float xCurr = xRefresh + min.x;
            while (xCurr < max.x)
            {
                float xP = ceilf(xCurr);   // 使用整数坐标使线条清晰
                draw->AddLine(ImVec2(xP, min.y), ImVec2(xP, max.y), 0x50FFFFFF);
                xCurr += xRefresh;
            }

            if (pValues_)
            {
                float currTotal = 0.0f;
                int mark = 0;
                for (int i = 0; i < NUM && mark < NUM_MARKERS; ++i)
                {
                    currTotal += pValues_[i];
                    if (total_ * markers[mark] < currTotal)
                    {
                        float xP = ceilf((float)(i + 1) / (float)NUM * (max.x - min.x) + min.x);
                        draw->AddLine(ImVec2(xP, min.y), ImVec2(xP, max.y), 0xFFFF0000);
                        ++mark;
                    }
                }
            }
        }

        void CalcHistogramSize(int numShown_)
        {
            ImVec2 wRegion = ImGui::GetContentRegionMax();
            // 替换已移除的 GetItemsLineHeightWithSpacing() 为 GetFrameHeightWithSpacing()
            float heightGone = 7.0f * ImGui::GetFrameHeightWithSpacing();
            wRegion.y -= heightGone;
            wRegion.y /= (float)numShown_;
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 pad = style.FramePadding;
            wRegion.x -= 2.0f * pad.x;
            size = wRegion;
        }

        void Draw(const char* name_, bool* pOpen_ = nullptr)
        {
            if (ImGui::Begin(name_, pOpen_))
            {
                int numShown = 0;
                if (ImGui::CollapsingHeader("Time Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", times, NUM, 0, nullptr, FLT_MAX, FLT_MAX, size);
                    PlotRefreshLines(timesTotal, times);
                }
                if (ImGui::CollapsingHeader("Count Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", counts, NUM, 0, nullptr, FLT_MAX, FLT_MAX, size);
                    PlotRefreshLines(countsTotal, counts);
                }
                if (ImGui::CollapsingHeader("Hitch Time Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", hitchTimes, NUM, 0, nullptr, FLT_MAX, FLT_MAX, size);
                    PlotRefreshLines();
                }
                if (ImGui::CollapsingHeader("Hitch Count Histogram"))
                {
                    ++numShown;
                    ImGui::PlotHistogram("", hitchCounts, NUM, 0, nullptr, FLT_MAX, FLT_MAX, size);
                    PlotRefreshLines();
                }
                if (ImGui::Button("Clear"))
                {
                    Clear();
                }
                CalcHistogramSize(numShown);
            }
            ImGui::End();
        }
    };

} // namespace ImGui