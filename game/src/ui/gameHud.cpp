#include "gameHud.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstdio>

// -------------------------------------------------------------------------------------------------------------------------

namespace UI
{
    namespace
    {
        // Layout dimensions are expressed at the reference resolution and scaled together.
        constexpr float c_referenceWidth        = 1280.0f;
        constexpr float c_referenceHeight       = 720.0f;
        constexpr float c_resourceColumnWidth   = 80.0f;
        constexpr float c_resourceBarWidth      = 56.0f;
        constexpr float c_slotSize              = 64.0f;
        constexpr float c_slotSpacing           = 8.0f;
        constexpr float c_resourceSpacing       = 16.0f;
        constexpr float c_panelPadding          = 8.0f;
        constexpr float c_resourceLabelOffset   = 70.0f;
        constexpr float c_xpOffset              = 90.0f;
        constexpr float c_xpHeight              = 20.0f;
        constexpr float c_bottomMargin          = 14.0f;

        constexpr std::array<const char*, 6> c_spellKeys = { "LMB", "RMB", "Q", "E", "R", "F" };

        constexpr float c_spellBarWidth = static_cast<float>(c_spellKeys.size()) * c_slotSize + static_cast<float>(c_spellKeys.size() - 1) * c_slotSpacing;
        constexpr float c_hudWidth      = 2.0f * (c_resourceColumnWidth + c_resourceSpacing) + c_spellBarWidth;
        constexpr float c_panelBottom   = c_xpOffset + c_xpHeight + c_panelPadding;

        enum class eBarDirection
        {
            Horizontal,
            Vertical
        };

        // -----------------------------------------------------------------------------------------------------------------

        float GetFraction(float _value, float _maximum)
        {
            if (_maximum <= 0.0f)
            {
                return 0.0f;
            }

            return std::clamp(_value / _maximum, 0.0f, 1.0f);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawText(ImDrawList& _rDrawList, const ImVec2& _rPosition, float _fontSize, const char* _pText)
        {
            _rDrawList.AddText(ImGui::GetFont(), _fontSize, ImVec2(_rPosition.x + 1.0f, _rPosition.y + 1.0f), IM_COL32(0, 0, 0, 230), _pText);
            _rDrawList.AddText(ImGui::GetFont(), _fontSize, _rPosition, IM_COL32(240, 241, 248, 255), _pText);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawBar(ImDrawList& _rDrawList, const ImVec2& _rPosition, const ImVec2& _rSize, float _fraction, ImU32 _color, float _scale, const char* _pLabel, eBarDirection _direction)
        {
            const ImVec2 end(_rPosition.x + _rSize.x, _rPosition.y + _rSize.y);
            _rDrawList.AddRectFilled(_rPosition, end, IM_COL32(14, 18, 28, 240), 4.0f * _scale);

            if (_fraction > 0.0f)
            {
                ImVec2 fillStart = _rPosition;
                ImVec2 fillEnd   = end;

                if (_direction == eBarDirection::Vertical)
                {
                    fillStart.y = end.y - _rSize.y * _fraction;
                }
                else
                {
                    fillEnd.x = _rPosition.x + _rSize.x * _fraction;
                }

                _rDrawList.AddRectFilled(fillStart, fillEnd, _color, 4.0f * _scale);
            }

            _rDrawList.AddRect(_rPosition, end, IM_COL32(125, 135, 158, 210), 4.0f * _scale);

            DrawText(_rDrawList, ImVec2(_rPosition.x + 6.0f * _scale, _rPosition.y + 4.0f * _scale), 13.0f * _scale, _pLabel);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawResourceBar(ImDrawList& _rDrawList, const ImVec2& _rColumnPosition, float _scale, const char* _pName, float _value, float _maximum, ImU32 _color)
        {
            const float  barInset = (c_resourceColumnWidth - c_resourceBarWidth) * 0.5f * _scale;

            const ImVec2 barPosition(_rColumnPosition.x + barInset, _rColumnPosition.y);
            const ImVec2 barSize(c_resourceBarWidth * _scale, c_slotSize * _scale);

            DrawBar(_rDrawList, barPosition, barSize, GetFraction(_value, _maximum), _color, _scale, "", eBarDirection::Vertical);

            char label[96];
            std::snprintf(label, sizeof(label), "%s %.0f/%.0f", _pName, _value, _maximum);
            const ImVec2 labelPosition(_rColumnPosition.x, _rColumnPosition.y + c_resourceLabelOffset * _scale);
            DrawText(_rDrawList, labelPosition, 12.0f * _scale, label);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawSpellIcon(ImDrawList& _rDrawList, const ImVec2& _rPosition, float _scale)
        {
            const ImVec2 center(_rPosition.x + 32.0f * _scale, _rPosition.y + 26.0f * _scale);
            const ImVec2 highlight(center.x - 3.0f * _scale, center.y - 3.0f * _scale);

            _rDrawList.AddCircleFilled(center, 18.0f * _scale, IM_COL32(67, 79, 155, 255));
            _rDrawList.AddCircleFilled(center, 12.0f * _scale, IM_COL32(131, 160, 255, 255));
            _rDrawList.AddCircleFilled(highlight, 5.0f * _scale, IM_COL32(220, 233, 255, 255));
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawSpellCooldown(ImDrawList& _rDrawList, const ImVec2& _rPosition, float _scale, float _remainingSeconds, float _cooldownFraction)
        {
            if (_cooldownFraction <= 0.0f)
            {
                return;
            }

            const float slotSize = c_slotSize * _scale;

            const ImVec2 slotEnd(_rPosition.x + slotSize, _rPosition.y + slotSize);
            const ImVec2 overlayStart(_rPosition.x, slotEnd.y - slotSize * _cooldownFraction);

            _rDrawList.AddRectFilled(overlayStart, slotEnd, IM_COL32(0, 0, 0, 185));

            char label[96];
            std::snprintf(label, sizeof(label), "%.1fs", _remainingSeconds);
            const ImVec2 labelPosition(_rPosition.x + 8.0f * _scale, _rPosition.y + 18.0f * _scale);
            DrawText(_rDrawList, labelPosition, 18.0f * _scale, label);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawSpellSlot(ImDrawList& _rDrawList, const ImVec2& _rPosition, float _scale, const char* _pKey, bool _hasSpell, const sHudState& _rState)
        {
            const ImVec2 slotEnd(_rPosition.x + c_slotSize * _scale, _rPosition.y + c_slotSize * _scale);
            _rDrawList.AddRectFilled(_rPosition, slotEnd, IM_COL32(34, 42, 70, 255), 6.0f * _scale);

            ImU32 borderColor = IM_COL32(89, 98, 122, 255);
            if (_hasSpell)
            {
                DrawSpellIcon(_rDrawList, _rPosition, _scale);

                const float cooldownFraction = GetFraction(_rState.spellCooldown, _rState.spellCooldownDuration);
                DrawSpellCooldown(_rDrawList, _rPosition, _scale, _rState.spellCooldown, cooldownFraction);
                borderColor = cooldownFraction > 0.0f ? IM_COL32(110, 117, 140, 255) : IM_COL32(174, 192, 255, 255);
            }

            _rDrawList.AddRect(_rPosition, slotEnd, borderColor, 6.0f * _scale);
            if (!_hasSpell)
            {
                const ImVec2 dashStart(_rPosition.x + 24.0f * _scale, _rPosition.y + 25.0f * _scale);
                const ImVec2 dashEnd(_rPosition.x + 40.0f * _scale, _rPosition.y + 25.0f * _scale);
                _rDrawList.AddLine(dashStart, dashEnd, borderColor);
            }

            const ImVec2 keyPosition(_rPosition.x + 8.0f * _scale, _rPosition.y + 45.0f * _scale);
            DrawText(_rDrawList, keyPosition, 13.0f * _scale, _pKey);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawExperienceBar(ImDrawList& _rDrawList, const ImVec2& _rPosition, float _scale, const sHudState& _rState)
        {
            char label[96];
            std::snprintf(label, sizeof(label), "Level %u    XP  %u / %u", _rState.level, _rState.xp, _rState.xpToNextLevel);

            const float progress = GetFraction(static_cast<float>(_rState.xp), static_cast<float>(_rState.xpToNextLevel));
            DrawBar(_rDrawList, _rPosition, ImVec2(c_hudWidth * _scale, c_xpHeight * _scale), progress, IM_COL32(194, 159, 72, 255), _scale, label, eBarDirection::Horizontal);
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cGameHud::Draw(const sHudState& _rState) const
    {
        const ImGuiViewport* pViewport = ImGui::GetMainViewport();
        if (pViewport == nullptr || pViewport->Size.x <= 0.0f || pViewport->Size.y <= 0.0f)
        {
            return;
        }

        // Draw-list primitives create no interactive windows and do not capture game input.
        ImDrawList* pDrawList = ImGui::GetBackgroundDrawList();
        if (pDrawList == nullptr)
        {
            return;
        }

        const float scale = std::min(pViewport->Size.x / c_referenceWidth, pViewport->Size.y / c_referenceHeight);
        const float width = c_hudWidth * scale;
        const float left  = pViewport->Pos.x + (pViewport->Size.x - width) * 0.5f;
        const float top   = pViewport->Pos.y + pViewport->Size.y - (c_panelBottom + c_bottomMargin) * scale;

        const ImVec2 panelStart(left - c_panelPadding * scale, top - c_panelPadding * scale);
        const ImVec2 panelEnd(left + width + c_panelPadding * scale, top + c_panelBottom * scale);
        pDrawList->AddRectFilled(panelStart, panelEnd, IM_COL32(9, 12, 20, 205), c_panelPadding * scale);

        DrawResourceBar(*pDrawList, ImVec2(left, top), scale, "HP", _rState.health, _rState.maxHealth, IM_COL32(178, 48, 65, 255));

        const float manaLeft = left + width - c_resourceColumnWidth * scale;
        DrawResourceBar(*pDrawList, ImVec2(manaLeft, top), scale, "Mana", _rState.mana, _rState.maxMana, IM_COL32(42, 100, 190, 255));

        const float spellsLeft = left + (c_resourceColumnWidth + c_resourceSpacing) * scale;
        for (size_t slotIndex = 0; slotIndex < c_spellKeys.size(); ++slotIndex)
        {
            const float slotLeft = spellsLeft + static_cast<float>(slotIndex) * (c_slotSize + c_slotSpacing) * scale;

            // Only LMB is connected to an existing spell; all other slots are display placeholders.
            const bool hasSpell = slotIndex == 0;

            DrawSpellSlot(*pDrawList, ImVec2(slotLeft, top), scale, c_spellKeys[slotIndex], hasSpell, _rState);
        }

        DrawExperienceBar(*pDrawList, ImVec2(left, top + c_xpOffset * scale), scale, _rState);
    }
}
