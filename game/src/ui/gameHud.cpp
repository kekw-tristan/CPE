#include "gameHud.h"

#include "../item/itemDatabase.h"
#include "../spells/spellManager.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstddef>
#include <cstdint>

// -------------------------------------------------------------------------------------------------------------------------

namespace UI
{

    // ---------------------------------------------------------------------------------------------------------------------

    namespace
    {

        // ---------------------------------------------------------------------------------------------------------------------

        // Layout dimensions are expressed at the reference resolution and scaled together.
        constexpr float c_referenceWidth        = 1280.0f;
        constexpr float c_referenceHeight       = 720.0f;
        constexpr float c_resourceOrbRadius     = 66.0f;
        constexpr float c_resourceEdgeInset     = 22.0f;
        constexpr int c_resourceRingSegments    = 48;
        constexpr float c_slotSize              = 64.0f;
        constexpr float c_slotSpacing           = 8.0f;
        constexpr float c_groupSpacing          = 12.0f;
        constexpr float c_xpHeight              = 22.0f;
        constexpr float c_xpSlotGap             = 8.0f;
        constexpr float c_bottomMargin          = 14.0f;
        constexpr float c_hudScale              = 0.84f;

        constexpr std::array<const char*, 6> c_spellKeys = { "LMB", "RMB", "Q", "E", "R", "F" };
        constexpr std::array<const char*, 4> c_usableKeys = { "1", "2", "3", "4" };

        constexpr float c_spellBarWidth         = static_cast<float>(c_spellKeys.size()) * c_slotSize + static_cast<float>(c_spellKeys.size() - 1) * c_slotSpacing;

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

        void DrawCenteredText(ImDrawList& _rDrawList, const ImVec2& _rCenter, float _fontSize, const char* _pText)
        {
            const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(_fontSize, FLT_MAX, 0.0f, _pText);
            DrawText(
                _rDrawList,
                ImVec2(_rCenter.x - textSize.x * 0.5f, _rCenter.y - textSize.y * 0.5f),
                _fontSize,
                _pText);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawBar(ImDrawList& _rDrawList, const ImVec2& _rPosition, const ImVec2& _rSize, float _fraction, ImU32 _color, float _scale, const char* _pLabel, eBarDirection _direction)
        {
            const ImVec2 end(_rPosition.x + _rSize.x, _rPosition.y + _rSize.y);

            _rDrawList.AddRectFilled(_rPosition, end, IM_COL32(13, 17, 23, 248), 5.0f * _scale);
            _rDrawList.AddRectFilled(
                ImVec2(_rPosition.x + 2.0f * _scale, _rPosition.y + 2.0f * _scale),
                ImVec2(end.x - 2.0f * _scale, end.y - 2.0f * _scale),
                IM_COL32(18, 23, 30, 245),
                3.0f * _scale);

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

            _rDrawList.AddRect(_rPosition, end, IM_COL32(66, 76, 91, 220), 5.0f * _scale);
            _rDrawList.AddRect(
                ImVec2(_rPosition.x + 1.0f * _scale, _rPosition.y + 1.0f * _scale),
                ImVec2(end.x - 1.0f * _scale, end.y - 1.0f * _scale),
                IM_COL32(112, 128, 154, 60),
                4.0f * _scale);

            DrawText(_rDrawList, ImVec2(_rPosition.x + 6.0f * _scale, _rPosition.y + 4.0f * _scale), 13.0f * _scale, _pLabel);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawResourceOrb(
            ImDrawList& _rDrawList,
            const ImVec2& _rCenter,
            float _scale,
            const char* _pName,
            float _value,
            float _maximum,
            ImU32 _color)
        {
            constexpr float c_twoPi = 6.28318530718f;

            const float radius = c_resourceOrbRadius * _scale;
            const float fraction = GetFraction(_value, _maximum);

            _rDrawList.AddCircleFilled(_rCenter, radius, IM_COL32(13, 17, 23, 248), c_resourceRingSegments);
            _rDrawList.AddCircle(_rCenter, radius, IM_COL32(55, 66, 82, 190), c_resourceRingSegments, 1.5f * _scale);
            _rDrawList.AddCircle(
                _rCenter,
                radius - 4.0f * _scale,
                IM_COL32(54, 65, 80, 180),
                c_resourceRingSegments,
                5.0f * _scale);

            if (fraction > 0.0f)
            {
                const int pointCount = std::max(2, static_cast<int>(std::ceil(fraction * static_cast<float>(c_resourceRingSegments))) + 1);
                std::array<ImVec2, c_resourceRingSegments + 1> points{};

                for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
                {
                    const float progress = static_cast<float>(pointIndex) / static_cast<float>(pointCount - 1);
                    const float angle = -0.25f * c_twoPi + fraction * c_twoPi * progress;
                    points[pointIndex] = ImVec2(
                        _rCenter.x + std::cos(angle) * (radius - 4.0f * _scale),
                        _rCenter.y + std::sin(angle) * (radius - 4.0f * _scale));
                }

                _rDrawList.AddPolyline(points.data(), pointCount, _color, 0, 5.0f * _scale);
            }

            _rDrawList.AddCircleFilled(_rCenter, radius - 8.0f * _scale, IM_COL32(18, 23, 30, 245), c_resourceRingSegments);

            char amount[32];
            std::snprintf(amount, sizeof(amount), "%.0f / %.0f", _value, _maximum);
            DrawCenteredText(_rDrawList, ImVec2(_rCenter.x, _rCenter.y - 11.0f * _scale), 11.0f * _scale, _pName);
            DrawCenteredText(_rDrawList, ImVec2(_rCenter.x, _rCenter.y + 8.0f * _scale), 12.0f * _scale, amount);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawSpellIcon(
            ImDrawList& _rDrawList,
            const ImVec2& _rPosition,
            float _scale,
            Gameplay::sItemId::Enum _spell)
        {
            const ImVec2 center(_rPosition.x + 32.0f * _scale, _rPosition.y + 26.0f * _scale);

            switch (_spell)
            {
                case Gameplay::sItemId::StoneShard:
                {
                    const ImVec2 points[3] =
                    {
                        ImVec2(center.x, center.y - 18.0f * _scale),
                        ImVec2(center.x + 15.0f * _scale, center.y + 14.0f * _scale),
                        ImVec2(center.x - 15.0f * _scale, center.y + 14.0f * _scale)
                    };

                    _rDrawList.AddTriangleFilled(points[0], points[1], points[2], IM_COL32(188, 104, 42, 255));
                    _rDrawList.AddTriangle(points[0], points[1], points[2], IM_COL32(255, 216, 148, 255), 2.0f * _scale);
                    break;
                }

                case Gameplay::sItemId::SporeOrb:
                    _rDrawList.AddCircleFilled(ImVec2(center.x, center.y - 5.0f * _scale), 15.0f * _scale, IM_COL32(68, 166, 64, 255));
                    _rDrawList.AddRectFilled(
                        ImVec2(center.x - 5.0f * _scale, center.y + 3.0f * _scale),
                        ImVec2(center.x + 5.0f * _scale, center.y + 16.0f * _scale),
                        IM_COL32(220, 211, 166, 255),
                        2.0f * _scale);
                    _rDrawList.AddCircleFilled(ImVec2(center.x - 5.0f * _scale, center.y - 7.0f * _scale), 2.0f * _scale, IM_COL32(209, 241, 152, 255));
                    _rDrawList.AddCircleFilled(ImVec2(center.x + 6.0f * _scale, center.y - 3.0f * _scale), 2.0f * _scale, IM_COL32(209, 241, 152, 255));
                    break;

                default:
                {
                    const ImVec2 highlight(center.x - 3.0f * _scale, center.y - 3.0f * _scale);
                    _rDrawList.AddCircleFilled(center, 18.0f * _scale, IM_COL32(67, 79, 155, 255));
                    _rDrawList.AddCircleFilled(center, 12.0f * _scale, IM_COL32(131, 160, 255, 255));
                    _rDrawList.AddCircleFilled(highlight, 5.0f * _scale, IM_COL32(220, 233, 255, 255));
                    break;
                }
            }
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

        void DrawSpellManaCost(ImDrawList& _rDrawList, const ImVec2& _rPosition, float _scale, float _manaCost)
        {
            char label[16];
            std::snprintf(label, sizeof(label), "%.0f", _manaCost);

            const float fontSize = 12.0f * _scale;
            const ImVec2 textSize = ImGui::CalcTextSize(label, nullptr, false, 0.0f);
            const ImVec2 labelPosition(
                _rPosition.x + c_slotSize * _scale - textSize.x - 5.0f * _scale,
                _rPosition.y + 4.0f * _scale);
            const ImVec2 backgroundStart(labelPosition.x - 3.0f * _scale, labelPosition.y - 1.0f * _scale);
            const ImVec2 backgroundEnd(
                labelPosition.x + textSize.x + 3.0f * _scale,
                labelPosition.y + fontSize + 1.0f * _scale);

            _rDrawList.AddRectFilled(backgroundStart, backgroundEnd, IM_COL32(19, 54, 104, 230), 3.0f * _scale);
            DrawText(_rDrawList, labelPosition, fontSize, label);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawSpellSlot(
            ImDrawList& _rDrawList,
            const ImVec2& _rPosition,
            float _scale,
            const char* _pKey,
            Gameplay::sItemId::Enum _spell,
            float _cooldown,
            float _cooldownDuration,
        float _manaCost)
        {
            const ImVec2 slotEnd(_rPosition.x + c_slotSize * _scale, _rPosition.y + c_slotSize * _scale);
            _rDrawList.AddRectFilled(_rPosition, slotEnd, IM_COL32(24, 29, 37, 255), 8.0f * _scale);
            _rDrawList.AddRectFilled(
                ImVec2(_rPosition.x + 2.0f * _scale, _rPosition.y + 2.0f * _scale),
                ImVec2(slotEnd.x - 2.0f * _scale, slotEnd.y - 2.0f * _scale),
                IM_COL32(27, 33, 42, 255),
                6.0f * _scale);

            ImU32 borderColor = IM_COL32(66, 76, 91, 255);
            if (_spell != Gameplay::sItemId::Undefined)
            {
                DrawSpellIcon(_rDrawList, _rPosition, _scale, _spell);

                const float cooldownFraction = GetFraction(_cooldown, _cooldownDuration);
                DrawSpellCooldown(_rDrawList, _rPosition, _scale, _cooldown, cooldownFraction);
                DrawSpellManaCost(_rDrawList, _rPosition, _scale, _manaCost);
                borderColor = cooldownFraction > 0.0f ? IM_COL32(89, 98, 112, 255) : IM_COL32(112, 128, 154, 255);
            }

            _rDrawList.AddRect(_rPosition, slotEnd, borderColor, 8.0f * _scale, 0, 1.5f * _scale);
            if (_spell == Gameplay::sItemId::Undefined)
            {
                const ImVec2 dashStart(_rPosition.x + 24.0f * _scale, _rPosition.y + 25.0f * _scale);
                const ImVec2 dashEnd(_rPosition.x + 40.0f * _scale, _rPosition.y + 25.0f * _scale);
                _rDrawList.AddLine(dashStart, dashEnd, borderColor);
            }

            _rDrawList.AddRectFilled(
                ImVec2(_rPosition.x + 4.0f * _scale, _rPosition.y + 42.0f * _scale),
                ImVec2(_rPosition.x + 39.0f * _scale, _rPosition.y + 61.0f * _scale),
                IM_COL32(18, 23, 30, 220),
                3.0f * _scale);
            const ImVec2 keyPosition(_rPosition.x + 8.0f * _scale, _rPosition.y + 45.0f * _scale);
            DrawText(_rDrawList, keyPosition, 13.0f * _scale, _pKey);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawUsableSlot(
            ImDrawList& _rDrawList,
            const ImVec2& _rPosition,
            float _scale,
            const char* _pKey,
            const sInventorySlotHudState& _rSlot)
        {
            const ImVec2 slotEnd(_rPosition.x + c_slotSize * _scale, _rPosition.y + c_slotSize * _scale);
            const ImU32 borderColor = _rSlot.item == Gameplay::sItemId::Undefined
                ? IM_COL32(66, 76, 91, 255)
                : IM_COL32(112, 128, 154, 255);

            _rDrawList.AddRectFilled(_rPosition, slotEnd, IM_COL32(24, 29, 37, 255), 8.0f * _scale);
            _rDrawList.AddRectFilled(
                ImVec2(_rPosition.x + 2.0f * _scale, _rPosition.y + 2.0f * _scale),
                ImVec2(slotEnd.x - 2.0f * _scale, slotEnd.y - 2.0f * _scale),
                IM_COL32(27, 33, 42, 255),
                6.0f * _scale);

            if (_rSlot.item == Gameplay::sItemId::Undefined)
            {
                const ImVec2 dashStart(_rPosition.x + 24.0f * _scale, _rPosition.y + 25.0f * _scale);
                const ImVec2 dashEnd(_rPosition.x + 40.0f * _scale, _rPosition.y + 25.0f * _scale);
                _rDrawList.AddLine(dashStart, dashEnd, borderColor);
            }
            else
            {
                const ImU32 potionColor = _rSlot.item == Gameplay::sItemId::HealthPotion
                    ? IM_COL32(190, 58, 72, 255)
                    : IM_COL32(50, 118, 214, 255);
                const ImVec2 bottleStart(_rPosition.x + 22.0f * _scale, _rPosition.y + 16.0f * _scale);
                const ImVec2 bottleEnd(_rPosition.x + 42.0f * _scale, _rPosition.y + 43.0f * _scale);

                _rDrawList.AddRectFilled(bottleStart, bottleEnd, potionColor, 4.0f * _scale);
                _rDrawList.AddRect(
                    bottleStart,
                    bottleEnd,
                    IM_COL32(229, 238, 255, 230),
                    4.0f * _scale,
                    0,
                    1.5f * _scale);
                _rDrawList.AddRectFilled(
                    ImVec2(_rPosition.x + 27.0f * _scale, _rPosition.y + 10.0f * _scale),
                    ImVec2(_rPosition.x + 37.0f * _scale, _rPosition.y + 18.0f * _scale),
                    potionColor,
                    2.0f * _scale);

                if (_rSlot.item == Gameplay::sItemId::HealthPotion)
                {
                    const ImVec2 center(_rPosition.x + 32.0f * _scale, _rPosition.y + 29.0f * _scale);
                    _rDrawList.AddLine(
                        ImVec2(center.x - 5.0f * _scale, center.y),
                        ImVec2(center.x + 5.0f * _scale, center.y),
                        IM_COL32(255, 238, 238, 255),
                        2.0f * _scale);
                    _rDrawList.AddLine(
                        ImVec2(center.x, center.y - 5.0f * _scale),
                        ImVec2(center.x, center.y + 5.0f * _scale),
                        IM_COL32(255, 238, 238, 255),
                        2.0f * _scale);
                }
                else if (_rSlot.item == Gameplay::sItemId::ManaPotion)
                {
                    _rDrawList.AddCircleFilled(
                        ImVec2(_rPosition.x + 32.0f * _scale, _rPosition.y + 29.0f * _scale),
                        5.0f * _scale,
                        IM_COL32(220, 240, 255, 255));
                }

                char amount[16];
                std::snprintf(amount, sizeof(amount), "%u", _rSlot.amount);
                const ImVec2 amountSize = ImGui::GetFont()->CalcTextSizeA(12.0f * _scale, FLT_MAX, 0.0f, amount);
                DrawText(
                    _rDrawList,
                    ImVec2(slotEnd.x - amountSize.x - 5.0f * _scale, slotEnd.y - amountSize.y - 4.0f * _scale),
                    12.0f * _scale,
                    amount);
            }

            _rDrawList.AddRect(_rPosition, slotEnd, borderColor, 8.0f * _scale, 0, 1.5f * _scale);
            _rDrawList.AddRectFilled(
                ImVec2(_rPosition.x + 4.0f * _scale, _rPosition.y + 42.0f * _scale),
                ImVec2(_rPosition.x + 25.0f * _scale, _rPosition.y + 61.0f * _scale),
                IM_COL32(18, 23, 30, 220),
                3.0f * _scale);
            DrawText(_rDrawList, ImVec2(_rPosition.x + 8.0f * _scale, _rPosition.y + 45.0f * _scale), 13.0f * _scale, _pKey);
        }

        // -----------------------------------------------------------------------------------------------------------------

        void DrawExperienceBar(
            ImDrawList& _rDrawList,
            const ImVec2& _rPosition,
            const ImVec2& _rSize,
            float _scale,
            const sHudState& _rState)
        {
            char label[96];
            std::snprintf(label, sizeof(label), "Level %u    XP  %u / %u", _rState.level, _rState.xp, _rState.xpToNextLevel);

            const float progress = GetFraction(static_cast<float>(_rState.xp), static_cast<float>(_rState.xpToNextLevel));
            DrawBar(_rDrawList, _rPosition, _rSize, progress, IM_COL32(194, 159, 72, 255), _scale, label, eBarDirection::Horizontal);
        }

        // -------------------------------------------------------------------------------------------------------------------------

        const char* GetAugmentDescription(Gameplay::sSpellAugment::Enum _augment)
        {
            switch (_augment)
            {
                case Gameplay::sSpellAugment::Multishot:
                    return "+1 Projektil pro Zauber";

                case Gameplay::sSpellAugment::Pierce:
                    return "+1 durchdrungenes Ziel";

                case Gameplay::sSpellAugment::DamageBonus:
                    return "+8 Schaden";

                case Gameplay::sSpellAugment::ExtraArea:
                    return "+0.2 Projektilradius";

                case Gameplay::sSpellAugment::ExtraDuration:
                    return "+0.5 Sekunden Dauer";

                case Gameplay::sSpellAugment::LowerCooldown:
                    return "-0.12 Sekunden Cooldown";

                default:
                    return "";
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------
        // Inventory
        // -------------------------------------------------------------------------------------------------------------------------

        ImU32 GetInventoryItemColor(Gameplay::sItemType::Enum _type)
        {
            switch (_type)
            {
                case Gameplay::sItemType::Armor:
                    return IM_COL32(126, 166, 200, 255);

                case Gameplay::sItemType::Usable:
                    return IM_COL32(176, 92, 99, 255);

                case Gameplay::sItemType::Spell:
                    return IM_COL32(126, 102, 220, 255);

                case Gameplay::sItemType::Item:
                    return IM_COL32(108, 180, 123, 255);

                default:
                    return IM_COL32(120, 125, 138, 255);
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void DrawCenteredInventoryText(
            ImDrawList& _rDrawList,
            const ImVec2& _rMin,
            const ImVec2& _rMax,
            float _fontSize,
            const char* _pText,
            ImU32 _color)
        {
            if (_pText == nullptr || _pText[0] == '\0')
                return;

            ImFont* pFont = ImGui::GetFont();

            const ImVec2 textSize = pFont->CalcTextSizeA(
                _fontSize,
                FLT_MAX,
                0.0f,
                _pText
            );

            const ImVec2 position(
                _rMin.x + (_rMax.x - _rMin.x - textSize.x) * 0.5f,
                _rMin.y + (_rMax.y - _rMin.y - textSize.y) * 0.5f
            );

            _rDrawList.AddText(
                pFont,
                _fontSize,
                position,
                _color,
                _pText
            );
        }

        // -------------------------------------------------------------------------------------------------------------------------

        void DrawInventoryGlyph(
            ImDrawList& _rDrawList,
            const ImVec2& _rCenter,
            float _radius,
            Gameplay::sItemType::Enum _type,
            ImU32 _color)
        {
            switch (_type)
            {
            case Gameplay::sItemType::Usable:
            {
                const float bottleWidth = _radius * 0.8f;
                const float bottleHeight = _radius * 1.25f;

                const ImVec2 min(
                    _rCenter.x - bottleWidth * 0.5f,
                    _rCenter.y - bottleHeight * 0.3f
                );

                const ImVec2 max(
                    _rCenter.x + bottleWidth * 0.5f,
                    _rCenter.y + bottleHeight * 0.55f
                );

                _rDrawList.AddRectFilled(
                    min,
                    max,
                    _color,
                    4.0f
                );

                _rDrawList.AddRectFilled(
                    ImVec2(_rCenter.x - bottleWidth * 0.22f, min.y - _radius * 0.35f),
                    ImVec2(_rCenter.x + bottleWidth * 0.22f, min.y + _radius * 0.05f),
                    _color,
                    2.0f
                );

                break;
            }

            case Gameplay::sItemType::Armor:
            {
                const ImVec2 points[5] =
                {
                    ImVec2(_rCenter.x, _rCenter.y - _radius),
                    ImVec2(_rCenter.x + _radius * 0.8f, _rCenter.y - _radius * 0.45f),
                    ImVec2(_rCenter.x + _radius * 0.6f, _rCenter.y + _radius * 0.55f),
                    ImVec2(_rCenter.x, _rCenter.y + _radius),
                    ImVec2(_rCenter.x - _radius * 0.6f, _rCenter.y + _radius * 0.55f)
                };

                _rDrawList.AddConvexPolyFilled(
                    points,
                    5,
                    _color
                );

                break;
            }

            case Gameplay::sItemType::Spell:
            {
                _rDrawList.AddCircleFilled(
                    _rCenter,
                    _radius,
                    _color,
                    24
                );

                _rDrawList.AddCircle(
                    _rCenter,
                    _radius * 0.62f,
                    IM_COL32(225, 230, 255, 220),
                    24,
                    2.0f
                );

                break;
            }

            default:
            {
                const ImVec2 points[4] =
                {
                    ImVec2(_rCenter.x, _rCenter.y - _radius),
                    ImVec2(_rCenter.x + _radius * 0.75f, _rCenter.y),
                    ImVec2(_rCenter.x, _rCenter.y + _radius),
                    ImVec2(_rCenter.x - _radius * 0.75f, _rCenter.y)
                };

                _rDrawList.AddConvexPolyFilled(
                    points,
                    4,
                    _color
                );

                break;
            }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------

        bool DrawInventorySlot(
            const char* _pId,
            const sInventorySlotHudState& _rSlot,
            const ImVec2& _rSize,
            float _scale,
            const char* _pHotkey = nullptr,
            bool _showName = true)
        {
            const ImVec2 slotMin = ImGui::GetCursorScreenPos();
            const ImVec2 slotMax(slotMin.x + _rSize.x, slotMin.y + _rSize.y);

            ImGui::InvisibleButton(_pId, _rSize);

            const bool hovered = ImGui::IsItemHovered();
            const bool clicked = ImGui::IsItemClicked();

            ImDrawList* pDrawList = ImGui::GetWindowDrawList();

            pDrawList->AddRectFilled(
                slotMin,
                slotMax,
                hovered
                ? IM_COL32(38, 45, 56, 255)
                : IM_COL32(24, 29, 37, 255),
                5.0f * _scale
            );

            pDrawList->AddRect(
                slotMin,
                slotMax,
                hovered
                ? IM_COL32(112, 128, 154, 255)
                : IM_COL32(66, 76, 91, 255),
                5.0f * _scale,
                0,
                hovered ? 2.0f : 1.0f
            );

            if (_rSlot.item != Gameplay::sItemId::Undefined)
            {
                const Gameplay::sItemDefinition& definition =
                    Gameplay::GetItemDefinition(_rSlot.item);

                const ImU32 itemColor =
                    GetInventoryItemColor(definition.type);

                const ImVec2 iconCenter(
                    slotMin.x + _rSize.x * 0.5f,
                    slotMin.y + _rSize.y * 0.38f
                );

                DrawInventoryGlyph(
                    *pDrawList,
                    iconCenter,
                    14.0f * _scale,
                    definition.type,
                    itemColor
                );

                if (_showName)
                {
                    const ImVec2 labelMin(
                        slotMin.x + 3.0f * _scale,
                        slotMin.y + _rSize.y - 22.0f * _scale
                    );

                    const ImVec2 labelMax(
                        slotMax.x - 3.0f * _scale,
                        slotMax.y - 4.0f * _scale
                    );

                    DrawCenteredInventoryText(
                        *pDrawList,
                        labelMin,
                        labelMax,
                        11.0f * _scale,
                        definition.pName,
                        IM_COL32(225, 228, 235, 255)
                    );
                }

                if (_rSlot.amount > 1)
                {
                    char amount[16];

                    std::snprintf(
                        amount,
                        sizeof(amount),
                        "%u",
                        _rSlot.amount
                    );

                    const ImVec2 textSize =
                        ImGui::GetFont()->CalcTextSizeA(
                            12.0f * _scale,
                            FLT_MAX,
                            0.0f,
                            amount
                        );

                    pDrawList->AddText(
                        ImGui::GetFont(),
                        12.0f * _scale,
                        ImVec2(
                            slotMax.x - textSize.x - 6.0f * _scale,
                            slotMax.y - textSize.y - 5.0f * _scale
                        ),
                        IM_COL32(245, 245, 245, 255),
                        amount
                    );
                }

                if (hovered)
                {
                    ImGui::BeginTooltip();

                    ImGui::TextUnformatted(definition.pName);

                    if (definition.maxStack > 1)
                        ImGui::Text("Amount: %u / %u", _rSlot.amount, definition.maxStack);

                    ImGui::EndTooltip();
                }
            }

            if (_pHotkey != nullptr)
            {
                pDrawList->AddText(
                    ImVec2(
                        slotMin.x + 6.0f * _scale,
                        slotMin.y + 5.0f * _scale
                    ),
                    IM_COL32(190, 195, 207, 255),
                    _pHotkey
                );
            }

            return clicked;
        }
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cGameHud::Draw(const sHudState& _rState)
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

        const float scale = c_hudScale * std::min(pViewport->Size.x / c_referenceWidth, pViewport->Size.y / c_referenceHeight);
        const ImVec2 center(pViewport->Pos.x + pViewport->Size.x * 0.5f, pViewport->Pos.y + pViewport->Size.y * 0.5f);
        const ImU32 reticleColor = _rState.anySpellOnCooldown
            ? IM_COL32(135, 145, 170, 230) : IM_COL32(205, 224, 255, 255);
        const std::array<ImVec2, 4> corners = {
            ImVec2(0.0f, -10.0f), ImVec2(10.0f, 0.0f),
            ImVec2(0.0f, 10.0f), ImVec2(-10.0f, 0.0f)
        };

        // Broken diamond with a clear center; dark outlines remain legible against the sky.
        for (size_t i = 0; i < corners.size(); ++i)
        {
            const ImVec2& corner = corners[i];
            const ImVec2& previous = corners[(i + 3) % corners.size()];
            const ImVec2& next = corners[(i + 1) % corners.size()];
            const std::array<ImVec2, 3> points = {
                ImVec2(center.x + (corner.x * 0.65f + previous.x * 0.35f) * scale,
                       center.y + (corner.y * 0.65f + previous.y * 0.35f) * scale),
                ImVec2(center.x + corner.x * scale, center.y + corner.y * scale),
                ImVec2(center.x + (corner.x * 0.65f + next.x * 0.35f) * scale,
                       center.y + (corner.y * 0.65f + next.y * 0.35f) * scale)
            };

            // Join both arms before drawing the outline so it cannot cover the bright corner.
            pDrawList->AddPolyline(points.data(), static_cast<int>(points.size()), IM_COL32(8, 12, 22, 240), 0, 4.0f * scale);
            pDrawList->AddPolyline(points.data(), static_cast<int>(points.size()), reticleColor, 0, 2.0f * scale);
        }

        constexpr std::array<const char*, 4> c_dungeonNames = {
            "Wurzelgruft - Uralter Kriecher", "Steinheiligtum - Waldkoloss",
            "Dornenbau - Dornenalpha", "Sporenkrypta - Sporenkoenig"
        };
        const ImVec2 questOrigin(pViewport->Pos.x + 18.0f * scale, pViewport->Pos.y + 18.0f * scale);
        //pDrawList->AddRectFilled(questOrigin, ImVec2(questOrigin.x + 380.0f * scale, questOrigin.y + 156.0f * scale), IM_COL32(9, 20, 12, 215), 6.0f * scale);
        unsigned int defeated = 0;

        //for (size_t i = 0; i < _rState.dungeons.size(); ++i)
        //{
        //    const auto& dungeon = _rState.dungeons[i];
        //    defeated += dungeon.defeated ? 1u : 0u;
        //    char label[128];
        //    std::snprintf(label, sizeof(label), "%s %s %.0fm %s%s", c_dungeonNames[i], dungeon.defeated ? "[OK]" : "", dungeon.distance, dungeon.offsetZ < 0.0f ? "S" : "N", dungeon.offsetX < 0.0f ? "W" : "O");
        //    const ImVec2 row(questOrigin.x + 8.0f * scale, questOrigin.y + (32.0f + static_cast<float>(i) * 24.0f) * scale);
        //    DrawText(*pDrawList, row, 13.0f * scale, label);
        //    if (dungeon.inArena && !dungeon.defeated)
        //        DrawBar(*pDrawList, ImVec2(row.x, row.y + 15.0f * scale), ImVec2(350.0f * scale, 5.0f * scale), dungeon.healthFraction, IM_COL32(180, 55, 40, 255), scale, "", eBarDirection::Horizontal);
        //}
        char progress[96];


        std::snprintf(progress, sizeof(progress), "Ring 1: Wald - Bosse %u/4%s", defeated, defeated == 4 ? " - Abgeschlossen!" : "");
        //DrawText(*pDrawList, ImVec2(questOrigin.x + 8.0f * scale, questOrigin.y + 8.0f * scale), 15.0f * scale, progress);
        //DrawText(*pDrawList, ImVec2(questOrigin.x + 8.0f * scale, questOrigin.y + 134.0f * scale), 12.0f * scale, "Eingang jeweils im Sueden (-Z). Flucht setzt Boss zurueck.");
        const float xpTop = pViewport->Pos.y + pViewport->Size.y - (c_xpHeight + c_bottomMargin) * scale;
        const float slotsTop = xpTop - (c_slotSize + c_xpSlotGap) * scale;
        const float resourceCenterY = slotsTop + c_slotSize * 0.5f * scale;
        const ImVec2 healthOrbCenter(
            pViewport->Pos.x + (c_resourceEdgeInset + c_resourceOrbRadius) * scale,
            resourceCenterY);
        const ImVec2 manaOrbCenter(
            pViewport->Pos.x + pViewport->Size.x - (c_resourceEdgeInset + c_resourceOrbRadius) * scale,
            resourceCenterY);
        const float usablesLeft = healthOrbCenter.x + (c_resourceOrbRadius + c_groupSpacing) * scale;
        const float spellsRight = manaOrbCenter.x - (c_resourceOrbRadius + c_groupSpacing) * scale;
        const float spellsLeft = spellsRight - c_spellBarWidth * scale;
        const float xpLeft = usablesLeft;

        DrawResourceOrb(*pDrawList, healthOrbCenter, scale, "HP", _rState.health, _rState.maxHealth, IM_COL32(194, 58, 76, 255));
        DrawResourceOrb(*pDrawList, manaOrbCenter, scale, "MANA", _rState.mana, _rState.maxMana, IM_COL32(54, 126, 224, 255));

        for (size_t slotIndex = 0; slotIndex < c_usableKeys.size(); ++slotIndex)
        {
            const float slotLeft = usablesLeft + static_cast<float>(slotIndex) * (c_slotSize + c_slotSpacing) * scale;
            DrawUsableSlot(
                *pDrawList,
                ImVec2(slotLeft, slotsTop),
                scale,
                c_usableKeys[slotIndex],
                _rState.inventory.usableSlots[slotIndex]);
        }

        for (size_t slotIndex = 0; slotIndex < c_spellKeys.size(); ++slotIndex)
        {
            const float slotLeft = spellsLeft + static_cast<float>(slotIndex) * (c_slotSize + c_slotSpacing) * scale;

            const Gameplay::sItemId::Enum spell = _rState.inventory.spellSlots[slotIndex].item;
            DrawSpellSlot(
                *pDrawList,
                ImVec2(slotLeft, slotsTop),
                scale,
                c_spellKeys[slotIndex],
                spell,
                _rState.spellCooldowns[slotIndex],
                _rState.spellCooldownDurations[slotIndex],
                _rState.spellManaCosts[slotIndex]);
        }

        DrawExperienceBar(
            *pDrawList,
            ImVec2(xpLeft, xpTop),
            ImVec2(spellsRight - usablesLeft, c_xpHeight * scale),
            scale,
            _rState);
        DrawInventory(_rState.inventory);
        DrawAugmentSelection(_rState.augmentSelection);
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cGameHud::DrawInventory(const sInventoryHudState& _rState)
    {
        if (!_rState.visible)
            return;

        const ImGuiViewport* pViewport = ImGui::GetMainViewport();

        if (pViewport == nullptr || pViewport->Size.x <= 0.0f || pViewport->Size.y <= 0.0f)
            return;

        const float scale = std::clamp(std::min(pViewport->Size.x / 1280.0f, pViewport->Size.y / 720.0f), 0.75f, 1.15f);

        constexpr float c_referenceWindowWidth = 920.0f;
        constexpr float c_referenceWindowHeight = 680.0f;

        const ImVec2 windowSize(c_referenceWindowWidth * scale, c_referenceWindowHeight * scale);
        const ImVec2 windowPosition(pViewport->Pos.x + (pViewport->Size.x - windowSize.x) * 0.5f, pViewport->Pos.y + (pViewport->Size.y - windowSize.y) * 0.5f);

        ImGui::SetNextWindowPos(windowPosition, ImGuiCond_Always);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * scale, 10.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * scale, 6.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f * scale, 4.0f * scale));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(13, 17, 23, 248));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(18, 23, 30, 245));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(55, 66, 82, 190));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(27, 33, 42, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(38, 46, 58, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(47, 57, 71, 255));
        ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(54, 65, 80, 180));

        const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings;

        if (!ImGui::Begin("Inventory", nullptr, windowFlags))
        {
            ImGui::End();

            ImGui::PopStyleColor(7);
            ImGui::PopStyleVar(6);

            return;
        }

        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        // ---------------------------------------------------------------------------------------------------------------------
        // Header
        // ---------------------------------------------------------------------------------------------------------------------

        const ImVec2 windowMin = ImGui::GetWindowPos();
        const ImVec2 windowMax(windowMin.x + ImGui::GetWindowWidth(), windowMin.y + ImGui::GetWindowHeight());

        const float headerHeight = 48.0f * scale;

        pDrawList->AddRectFilled(windowMin, ImVec2(windowMax.x, windowMin.y + headerHeight), IM_COL32(18, 23, 30, 255), 7.0f * scale, ImDrawFlags_RoundCornersTop);
        pDrawList->AddLine(ImVec2(windowMin.x + 1.0f, windowMin.y + headerHeight), ImVec2(windowMax.x - 1.0f, windowMin.y + headerHeight), IM_COL32(58, 69, 84, 210), 1.0f);

        pDrawList->AddText(ImGui::GetFont(), 22.0f * scale, ImVec2(windowMin.x + 19.0f * scale, windowMin.y + 12.0f * scale), IM_COL32(235, 238, 244, 255), "Inventory");

        ImGui::SetCursorPosY(headerHeight + 11.0f * scale);

        // ---------------------------------------------------------------------------------------------------------------------
        // Upper section
        // ---------------------------------------------------------------------------------------------------------------------

        const float upperHeight = 252.0f * scale;
        const float armorWidth = 296.0f * scale;

        const ImVec2 upperAvailable = ImGui::GetContentRegionAvail();
        const float abilitiesWidth = upperAvailable.x - armorWidth - 8.0f * scale;

        // ---------------------------------------------------------------------------------------------------------------------
        // Spells + Usables
        // ---------------------------------------------------------------------------------------------------------------------

        const ImVec2 abilitiesPanelMin = ImGui::GetCursorScreenPos();
        const ImVec2 abilitiesPanelMax(abilitiesPanelMin.x + abilitiesWidth, abilitiesPanelMin.y + upperHeight);

        pDrawList->AddRectFilled(abilitiesPanelMin, abilitiesPanelMax, IM_COL32(18, 23, 30, 235), 6.0f * scale);
        pDrawList->AddRect(abilitiesPanelMin, abilitiesPanelMax, IM_COL32(48, 58, 72, 175), 6.0f * scale);

        ImGui::BeginChild("InventoryAbilities", ImVec2(abilitiesWidth, upperHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(10.0f * scale, 8.0f * scale));
        ImGui::TextUnformatted("Spells");

        ImGui::SetCursorPosX(10.0f * scale);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f * scale);
        ImGui::Separator();

        constexpr std::array<const char*, 6> c_spellKeys = { "LMB", "RMB", "Q", "E", "R", "F" };

        const float spellSpacing = 7.0f * scale;
        const float spellAreaWidth = ImGui::GetContentRegionAvail().x - 10.0f * scale;
        const float spellSlotWidth = (spellAreaWidth - spellSpacing * static_cast<float>(c_spellKeys.size() - 1)) / static_cast<float>(c_spellKeys.size());

        const ImVec2 spellSlotSize(spellSlotWidth, 75.0f * scale);

        ImGui::SetCursorPosX(10.0f * scale);

        for (size_t slotIndex = 0; slotIndex < _rState.spellSlots.size(); ++slotIndex)
        {
            ImGui::PushID(static_cast<int>(3000 + slotIndex));

            const sInventorySlotHudState& slot = _rState.spellSlots[slotIndex];

            DrawInventorySlot("SpellSlot", slot, spellSlotSize, scale, c_spellKeys[slotIndex], false);

            if (slot.item != Gameplay::sItemId::Undefined && ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("SpellSlot", &slotIndex, sizeof(slotIndex));
                ImGui::TextUnformatted(Gameplay::GetItemDefinition(slot.item).pName);
                ImGui::EndDragDropSource();
            }

            AcceptInventorySlotDrop(_rState, eInventoryDropTarget::Spell, slotIndex);

            ImGui::PopID();

            if (slotIndex + 1 < _rState.spellSlots.size())
                ImGui::SameLine(0.0f, spellSpacing);
        }

        ImGui::SetCursorPosX(10.0f * scale);
        ImGui::SetCursorPosY(126.0f * scale);

        ImGui::TextUnformatted("Usables");

        ImGui::SetCursorPosX(10.0f * scale);
        ImGui::Separator();

        const float usableSpacing = 8.0f * scale;
        const float usableAreaWidth = ImGui::GetContentRegionAvail().x - 10.0f * scale;
        const float usableSlotWidth = (usableAreaWidth - usableSpacing * static_cast<float>(c_usableKeys.size() - 1)) / static_cast<float>(c_usableKeys.size());

        const ImVec2 usableSlotSize(usableSlotWidth, 76.0f * scale);

        ImGui::SetCursorPosX(10.0f * scale);

        for (size_t slotIndex = 0; slotIndex < _rState.usableSlots.size(); ++slotIndex)
        {
            ImGui::PushID(static_cast<int>(2000 + slotIndex));

            const sInventorySlotHudState& slot = _rState.usableSlots[slotIndex];

            DrawInventorySlot("UsableSlot", slot, usableSlotSize, scale, c_usableKeys[slotIndex], false);

            if (slot.item != Gameplay::sItemId::Undefined && ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("UsableSlot", &slotIndex, sizeof(slotIndex));
                ImGui::TextUnformatted(Gameplay::GetItemDefinition(slot.item).pName);
                ImGui::EndDragDropSource();
            }

            AcceptInventorySlotDrop(_rState, eInventoryDropTarget::Usable, slotIndex);

            ImGui::PopID();

            if (slotIndex + 1 < _rState.usableSlots.size())
                ImGui::SameLine(0.0f, usableSpacing);
        }

        ImGui::EndChild();

        ImGui::SameLine(0.0f, 8.0f * scale);

        // ---------------------------------------------------------------------------------------------------------------------
        // Armor
        // ---------------------------------------------------------------------------------------------------------------------

        const ImVec2 armorPanelMin = ImGui::GetCursorScreenPos();
        const ImVec2 armorPanelMax(armorPanelMin.x + armorWidth, armorPanelMin.y + upperHeight);

        pDrawList->AddRectFilled(armorPanelMin, armorPanelMax, IM_COL32(18, 23, 30, 235), 6.0f * scale);
        pDrawList->AddRect(armorPanelMin, armorPanelMax, IM_COL32(48, 58, 72, 175), 6.0f * scale);

        ImGui::BeginChild("InventoryArmor", ImVec2(armorWidth, upperHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(10.0f * scale, 8.0f * scale));

        ImGui::TextUnformatted("Armor");

        ImGui::SetCursorPosX(10.0f * scale);
        ImGui::Separator();

        constexpr std::array<const char*, Gameplay::sArmorSlot::NumberOfElements> c_armorNames = { "Head", "Chest", "Ring", "Legs", "Boots" };

        const float armorRowHeight = 30.0f * scale;
        const float armorRowSpacing = 4.0f * scale;
        const float armorRowWidth = armorWidth - 20.0f * scale;

        for (size_t slotIndex = 0; slotIndex < _rState.armorSlots.size(); ++slotIndex)
        {
            const sInventorySlotHudState& slot = _rState.armorSlots[slotIndex];

            ImGui::PushID(static_cast<int>(1000 + slotIndex));

            ImGui::SetCursorPosX(10.0f * scale);

            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            const ImVec2 rowMax(rowMin.x + armorRowWidth, rowMin.y + armorRowHeight);

            // Submit the actual ImGui item first.
            ImGui::InvisibleButton("ArmorSlot", ImVec2(armorRowWidth, armorRowHeight));

            const bool hovered = ImGui::IsItemHovered();

            if (slot.item != Gameplay::sItemId::Undefined && ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("ArmorSlot", &slotIndex, sizeof(slotIndex));
                ImGui::TextUnformatted(Gameplay::GetItemDefinition(slot.item).pName);
                ImGui::EndDragDropSource();
            }

            AcceptInventorySlotDrop(_rState, eInventoryDropTarget::Armor, slotIndex);

            pDrawList->AddRectFilled(rowMin, rowMax, hovered ? IM_COL32(34, 41, 52, 255) : IM_COL32(25, 31, 39, 255), 4.0f * scale);
            pDrawList->AddRect(rowMin, rowMax, hovered ? IM_COL32(87, 101, 122, 220) : IM_COL32(50, 60, 74, 180), 4.0f * scale);

            const float iconSize = 24.0f * scale;

            const ImVec2 iconMin(rowMin.x + 4.0f * scale, rowMin.y + (armorRowHeight - iconSize) * 0.5f);
            const ImVec2 iconMax(iconMin.x + iconSize, iconMin.y + iconSize);

            pDrawList->AddRectFilled(iconMin, iconMax, IM_COL32(32, 39, 48, 255), 3.0f * scale);
            pDrawList->AddRect(iconMin, iconMax, IM_COL32(62, 73, 88, 210), 3.0f * scale);

            const char* pItemName = "Empty";

            if (slot.item != Gameplay::sItemId::Undefined)
            {
                const Gameplay::sItemDefinition& definition = Gameplay::GetItemDefinition(slot.item);

                pItemName = definition.pName;

                DrawInventoryGlyph(
                    *pDrawList,
                    ImVec2(iconMin.x + iconSize * 0.5f, iconMin.y + iconSize * 0.5f),
                    7.5f * scale,
                    definition.type,
                    GetInventoryItemColor(definition.type)
                );
            }

            const float textLeft = iconMax.x + 8.0f * scale;

            pDrawList->AddText(
                ImGui::GetFont(),
                9.5f * scale,
                ImVec2(textLeft, rowMin.y + 2.0f * scale),
                IM_COL32(126, 136, 152, 255),
                c_armorNames[slotIndex]
            );

            pDrawList->AddText(
                ImGui::GetFont(),
                12.0f * scale,
                ImVec2(textLeft, rowMin.y + 14.0f * scale),
                slot.item == Gameplay::sItemId::Undefined ? IM_COL32(91, 99, 112, 255) : IM_COL32(219, 225, 234, 255),
                pItemName
            );

            if (slot.item != Gameplay::sItemId::Undefined && hovered)
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(Gameplay::GetItemDefinition(slot.item).pName);
                ImGui::EndTooltip();
            }

            ImGui::PopID();

            // Use a real ImGui item for spacing instead of manually extending the cursor.
            if (slotIndex + 1 < _rState.armorSlots.size())
                ImGui::Dummy(ImVec2(0.0f, armorRowSpacing));
        }

        ImGui::EndChild();

        // ---------------------------------------------------------------------------------------------------------------------
        // Items
        // ---------------------------------------------------------------------------------------------------------------------

        ImGui::SetCursorPosY(headerHeight + 11.0f * scale + upperHeight + 10.0f * scale);

        const ImVec2 itemsPanelMin = ImGui::GetCursorScreenPos();
        const ImVec2 remaining = ImGui::GetContentRegionAvail();
        const ImVec2 itemsPanelMax(itemsPanelMin.x + remaining.x, itemsPanelMin.y + remaining.y);

        pDrawList->AddRectFilled(itemsPanelMin, itemsPanelMax, IM_COL32(18, 23, 30, 235), 6.0f * scale);
        pDrawList->AddRect(itemsPanelMin, itemsPanelMax, IM_COL32(48, 58, 72, 175), 6.0f * scale);

        ImGui::BeginChild("InventoryItems", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPos(ImVec2(10.0f * scale, 8.0f * scale));

        ImGui::TextUnformatted("Items");

        size_t usedSlots = 0;

        for (const sInventorySlotHudState& slot : _rState.inventorySlots)
        {
            if (slot.item != Gameplay::sItemId::Undefined)
                ++usedSlots;
        }

        char capacityLabel[32];

        std::snprintf(capacityLabel, sizeof(capacityLabel), "%zu / %zu", usedSlots, _rState.inventorySlots.size());

        const float capacityWidth = ImGui::CalcTextSize(capacityLabel).x;

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - capacityWidth - 12.0f * scale);
        ImGui::TextDisabled("%s", capacityLabel);

        ImGui::SetCursorPosX(10.0f * scale);
        ImGui::Separator();

        constexpr size_t c_columns = 6;
        constexpr size_t c_rows = 4;

        const float gridSpacing = 7.0f * scale;
        const float horizontalInset = 10.0f * scale;
        const float gridWidth = ImGui::GetWindowWidth() - horizontalInset * 2.0f;
        const float slotWidth = (gridWidth - gridSpacing * static_cast<float>(c_columns - 1)) / static_cast<float>(c_columns);

        const float gridTop = ImGui::GetCursorPosY();
        const float gridBottomInset = 10.0f * scale;
        const float gridHeight = ImGui::GetWindowHeight() - gridTop - gridBottomInset;
        const float slotHeight = (gridHeight - gridSpacing * static_cast<float>(c_rows - 1)) / static_cast<float>(c_rows);

        const ImVec2 itemSlotSize(slotWidth, slotHeight);

        ImGui::SetCursorPosX(horizontalInset);

        for (size_t slotIndex = 0; slotIndex < _rState.inventorySlots.size(); ++slotIndex)
        {
            ImGui::PushID(static_cast<int>(slotIndex));

            const sInventorySlotHudState& slot = _rState.inventorySlots[slotIndex];

            DrawInventorySlot("ItemSlot", slot, itemSlotSize, scale);

            if (slot.item != Gameplay::sItemId::Undefined && ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("InventorySlot", &slotIndex, sizeof(slotIndex));
                ImGui::TextUnformatted(Gameplay::GetItemDefinition(slot.item).pName);
                ImGui::EndDragDropSource();
            }

            AcceptInventorySlotDrop(_rState, eInventoryDropTarget::Inventory, slotIndex);

            ImGui::PopID();

            if ((slotIndex + 1) % c_columns != 0)
            {
                ImGui::SameLine(0.0f, gridSpacing);
            }
            else if (slotIndex + 1 < _rState.inventorySlots.size())
            {
                ImGui::SetCursorPosX(horizontalInset);
            }
        }

        ImGui::EndChild();

        ImGui::End();

        ImGui::PopStyleColor(7);
        ImGui::PopStyleVar(6);
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cGameHud::ConsumeInventoryAction(
        eInventoryAction& _rAction,
        size_t& _rSourceSlot,
        size_t& _rDestinationSlot)
    {
        if (!m_hasInventoryAction)
            return false;

        _rAction = m_inventoryAction;
        _rSourceSlot = m_sourceInventorySlot;
        _rDestinationSlot = m_destinationInventorySlot;
        m_hasInventoryAction = false;

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    bool cGameHud::ConsumeAugmentSelection(Gameplay::sSpellAugment::Enum& _rAugment)
    {
        if (!m_hasAugmentSelection)
            return false;

        _rAugment = m_augmentSelection;
        m_hasAugmentSelection = false;
        m_augmentSelection = Gameplay::sSpellAugment::Undefined;

        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cGameHud::DrawAugmentSelection(const sAugmentHudState& _rState)
    {
        if (!_rState.visible)
            return;

        const ImGuiViewport* pViewport = ImGui::GetMainViewport();
        if (pViewport == nullptr)
            return;

        const float scale = std::clamp(std::min(pViewport->Size.x / c_referenceWidth, pViewport->Size.y / c_referenceHeight), 0.7f, 1.1f);
        const ImVec2 windowSize(900.0f * scale, 360.0f * scale);

        ImGui::SetNextWindowPos(
            ImVec2(pViewport->Pos.x + pViewport->Size.x * 0.5f, pViewport->Pos.y + pViewport->Size.y * 0.5f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 7.0f * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f * scale, 24.0f * scale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f * scale, 10.0f * scale));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(12, 16, 29, 250));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(116, 92, 190, 210));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(25, 27, 50, 255));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(67, 54, 119, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(93, 76, 157, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(122, 99, 195, 255));

        const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        if (!ImGui::Begin("AugmentSelection", nullptr, windowFlags))
        {
            ImGui::End();
            ImGui::PopStyleColor(6);
            ImGui::PopStyleVar(5);
            return;
        }

        ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("LEVEL UP").x) * 0.5f);
        ImGui::TextColored(ImVec4(0.89f, 0.80f, 1.00f, 1.00f), "LEVEL UP");
        ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("Waehle eine Augmentierung fuer alle Zauber.").x) * 0.5f);
        ImGui::TextDisabled("Waehle eine Augmentierung fuer alle Zauber.");
        if (_rState.selectionsRemaining > 1)
            ImGui::TextColored(ImVec4(0.93f, 0.79f, 0.44f, 1.00f), "%u weitere Auswahlen offen", _rState.selectionsRemaining - 1);

        ImGui::Spacing();
        const float cardSpacing = 12.0f * scale;
        const float cardWidth = (ImGui::GetContentRegionAvail().x - cardSpacing * 2.0f) / 3.0f;
        const ImVec2 cardSize(cardWidth, 210.0f * scale);

        for (size_t choiceIndex = 0; choiceIndex < _rState.choices.size(); ++choiceIndex)
        {
            const Gameplay::sSpellAugment::Enum augment = _rState.choices[choiceIndex];
            if (augment == Gameplay::sSpellAugment::Undefined)
                continue;

            const Gameplay::sSpellAugmentDefinition& definition = Gameplay::SpellManager::GetAugment(augment);
            const uint8_t stackCount = _rState.stackCounts[choiceIndex];

            ImGui::PushID(static_cast<int>(choiceIndex));
            ImGui::BeginChild("AugmentCard", cardSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::TextColored(ImVec4(0.86f, 0.79f, 1.00f, 1.00f), "%s", definition.pName);
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("%s", GetAugmentDescription(augment));
            ImGui::SetCursorPosY(cardSize.y - 68.0f * scale);
            ImGui::TextDisabled("Stack %u / %u", stackCount, definition.maxStacks);

            if (ImGui::Button("Auswaehlen", ImVec2(-1.0f, 32.0f * scale)))
            {
                m_augmentSelection = augment;
                m_hasAugmentSelection = true;
            }
            ImGui::EndChild();
            ImGui::PopID();

            if (choiceIndex + 1 < _rState.choices.size())
                ImGui::SameLine(0.0f, cardSpacing);
        }

        ImGui::End();
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(5);
    }

    // ---------------------------------------------------------------------------------------------------------------------

    void cGameHud::AcceptInventorySlotDrop(
        const sInventoryHudState& _rState,
        eInventoryDropTarget _target,
        size_t _destinationSlot)
    {
        if (!ImGui::BeginDragDropTarget())
            return;

        const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("InventorySlot");
        eInventoryAction action = eInventoryAction::MoveItem;

        if (pPayload != nullptr && pPayload->DataSize == sizeof(size_t))
        {
            size_t sourceSlot = 0;
            std::memcpy(&sourceSlot, pPayload->Data, sizeof(sourceSlot));

            if (sourceSlot < _rState.inventorySlots.size())
            {
                const sInventorySlotHudState& source = _rState.inventorySlots[sourceSlot];
                bool acceptsItem = source.item != Gameplay::sItemId::Undefined;

                if (acceptsItem)
                {
                    const Gameplay::sItemDefinition& definition = Gameplay::GetItemDefinition(source.item);

                    switch (_target)
                    {
                        case eInventoryDropTarget::Inventory:
                            acceptsItem = sourceSlot != _destinationSlot;
                            break;

                        case eInventoryDropTarget::Armor:
                            action = eInventoryAction::EquipArmor;
                            acceptsItem = definition.type == Gameplay::sItemType::Armor &&
                                static_cast<size_t>(definition.armorSlot) == _destinationSlot;
                            break;

                        case eInventoryDropTarget::Usable:
                            action = eInventoryAction::EquipUsable;
                            acceptsItem = definition.type == Gameplay::sItemType::Usable;
                            break;

                        case eInventoryDropTarget::Spell:
                            action = eInventoryAction::EquipSpell;
                            acceptsItem = definition.type == Gameplay::sItemType::Spell;
                            break;
                    }
                }

                if (acceptsItem)
                {
                    m_inventoryAction = action;
                    m_sourceInventorySlot = sourceSlot;
                    m_destinationInventorySlot = _destinationSlot;
                    m_hasInventoryAction = true;
                }
            }
        }

        if (pPayload == nullptr && _target == eInventoryDropTarget::Spell)
        {
            pPayload = ImGui::AcceptDragDropPayload("SpellSlot");

            if (pPayload != nullptr && pPayload->DataSize == sizeof(size_t))
            {
                size_t sourceSlot = 0;
                std::memcpy(&sourceSlot, pPayload->Data, sizeof(sourceSlot));

                if (sourceSlot < _rState.spellSlots.size() && sourceSlot != _destinationSlot
                    && _rState.spellSlots[sourceSlot].item != Gameplay::sItemId::Undefined)
                {
                    m_inventoryAction = eInventoryAction::MoveSpell;
                    m_sourceInventorySlot = sourceSlot;
                    m_destinationInventorySlot = _destinationSlot;
                    m_hasInventoryAction = true;
                }
            }
        }

        if (pPayload == nullptr && _target == eInventoryDropTarget::Usable)
        {
            pPayload = ImGui::AcceptDragDropPayload("UsableSlot");

            if (pPayload != nullptr && pPayload->DataSize == sizeof(size_t))
            {
                size_t sourceSlot = 0;
                std::memcpy(&sourceSlot, pPayload->Data, sizeof(sourceSlot));

                if (sourceSlot < _rState.usableSlots.size() && sourceSlot != _destinationSlot
                    && _rState.usableSlots[sourceSlot].item != Gameplay::sItemId::Undefined)
                {
                    m_inventoryAction = eInventoryAction::MoveUsable;
                    m_sourceInventorySlot = sourceSlot;
                    m_destinationInventorySlot = _destinationSlot;
                    m_hasInventoryAction = true;
                }
            }
        }

        if (pPayload == nullptr && _target == eInventoryDropTarget::Inventory)
        {
            pPayload = ImGui::AcceptDragDropPayload("ArmorSlot");
            action = eInventoryAction::UnequipArmor;

            if (pPayload == nullptr)
            {
                pPayload = ImGui::AcceptDragDropPayload("UsableSlot");
                action = eInventoryAction::UnequipUsable;
            }

            if (pPayload == nullptr)
            {
                pPayload = ImGui::AcceptDragDropPayload("SpellSlot");
                action = eInventoryAction::UnequipSpell;
            }

            if (pPayload != nullptr && pPayload->DataSize == sizeof(size_t))
            {
                size_t sourceSlot = 0;
                std::memcpy(&sourceSlot, pPayload->Data, sizeof(sourceSlot));

                bool acceptsItem = false;

                switch (action)
                {
                    case eInventoryAction::UnequipArmor:
                        acceptsItem = sourceSlot < _rState.armorSlots.size() &&
                            _rState.armorSlots[sourceSlot].item != Gameplay::sItemId::Undefined;
                        break;

                    case eInventoryAction::UnequipUsable:
                        acceptsItem = sourceSlot < _rState.usableSlots.size() &&
                            _rState.usableSlots[sourceSlot].item != Gameplay::sItemId::Undefined;
                        break;

                    case eInventoryAction::UnequipSpell:
                        acceptsItem = sourceSlot < _rState.spellSlots.size() &&
                            _rState.spellSlots[sourceSlot].item != Gameplay::sItemId::Undefined;
                        break;

                    default:
                        break;
                }

                if (acceptsItem)
                {
                    m_inventoryAction = action;
                    m_sourceInventorySlot = sourceSlot;
                    m_destinationInventorySlot = _destinationSlot;
                    m_hasInventoryAction = true;
                }
            }
        }

        ImGui::EndDragDropTarget();
    }

    // ---------------------------------------------------------------------------------------------------------------------

}

// ---------------------------------------------------------------------------------------------------------------------
