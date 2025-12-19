
//
// Created by gogop on 8/2/2025.
//

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
// Color utilities for ANSI escape sequences
#define MAKE_RGB_COLOR(r, g, b) "\033[38;2;" #r ";" #g ";" #b "m"
#define MAKE_BG_RGB_COLOR(r, g, b) "\033[48;2;" #r ";" #g ";" #b "m"
#define RESET_COLOR "\033[0m"

// HSL to RGB conversion function
constexpr void hslToRgb(float h, float s, float l, uint8_t& r, uint8_t& g, uint8_t& b) {
	h = std::fmod(h, 360.0f);
	s = std::clamp(s, 0.0f, 1.0f);
	l = std::clamp(l, 0.0f, 1.0f);

	float c = (1 - std::abs(2 * l - 1)) * s;
	float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2) - 1));
	float m = l - c / 2;

	float r1, g1, b1;
	if (h < 60) { r1 = c; g1 = x; b1 = 0; }
	else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
	else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
	else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
	else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
	else { r1 = c; g1 = 0; b1 = x; }

	r = static_cast<uint8_t>((r1 + m) * 255);
	g = static_cast<uint8_t>((g1 + m) * 255);
	b = static_cast<uint8_t>((b1 + m) * 255);
}

// Macro for HSL colors
#define MAKE_HSL_COLOR(h, s, l)  [&]() { \
    uint8_t r, g, b; \
    hslToRgb(h, s, l, r, g, b); \
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m"; \
}()

#define MAKE_BG_HSL_COLOR(h, s, l) [&]() { \
    uint8_t r, g, b; \
    hslToRgb(h, s, l, r, g, b); \
    return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m"; \
}()

// Style modifiers
#define BOLD_TEXT "\033[1m"
#define DIM_TEXT "\033[2m"
#define ITALIC_TEXT "\033[3m"
#define UNDERLINE_TEXT "\033[4m"
#define BLINK_TEXT "\033[5m"

// Simple colors
#define RED_TEXT "\033[0;31m"
#define YELLOW_TEXT "\033[0;33m"

#define CL_ORANGE MAKE_RGB_COLOR(204, 120, 50)
#define CL_YELLOW MAKE_RGB_COLOR(255, 188, 89)
#define CL_WHITE MAKE_RGB_COLOR(169, 183, 198)
#define CL_LIGHT_PURPLE MAKE_RGB_COLOR(129, 172, 226)
