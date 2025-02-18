//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "c_point_worldtext.h"
#include "model_types.h"
#include "clienteffectprecachesystem.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#pragma optimize("", off)

typedef struct Character {
  int codePoint, x, y, width, height, originX, originY, advance;
} Character;

typedef struct Font {
  const char *name;
  int size, bold, italic, width, height, characterCount;
  Character *characters;
} Font;

static Character characters_TF2_Build[] = {
  {' ', 146, 238, 12, 12, 6, 6, 25},
  {'!', 67, 182, 24, 56, 6, 50, 16},
  {'"', 0, 238, 24, 24, 6, 54, 15},
  {'#', 609, 182, 51, 50, 6, 47, 42},
  {'$', 477, 182, 34, 54, 6, 49, 25},
  {'%', 564, 182, 45, 51, 6, 47, 36},
  {'&', 636, 0, 49, 58, 6, 50, 39},
  {'\'', 1000, 182, 19, 25, 6, 54, 10},
  {'(', 118, 0, 34, 62, 6, 54, 25},
  {')', 152, 0, 34, 62, 6, 54, 25},
  {'*', 861, 182, 34, 34, 6, 54, 25},
  {'+', 782, 182, 36, 37, 6, 40, 27},
  {',', 978, 182, 22, 26, 6, 16, 12},
  {'-', 70, 238, 32, 19, 6, 30, 23},
  {'.', 24, 238, 23, 22, 6, 16, 14},
  {'/', 373, 182, 36, 55, 6, 50, 27},
  {'0', 91, 182, 52, 55, 6, 50, 43},
  {'1', 409, 182, 32, 55, 5, 50, 25},
  {'2', 239, 182, 46, 55, 6, 50, 37},
  {'3', 192, 182, 47, 55, 6, 50, 37},
  {'4', 143, 182, 49, 55, 6, 50, 41},
  {'5', 330, 182, 43, 55, 6, 50, 34},
  {'6', 0, 182, 40, 56, 6, 51, 30},
  {'7', 285, 182, 45, 55, 6, 50, 36},
  {'8', 961, 125, 45, 56, 6, 50, 36},
  {'9', 873, 0, 42, 58, 6, 50, 32},
  {':', 760, 182, 22, 43, 6, 41, 14},
  {';', 738, 182, 22, 48, 6, 42, 13},
  {'<', 660, 182, 39, 49, 6, 46, 30},
  {'=', 895, 182, 39, 28, 6, 36, 29},
  {'>', 699, 182, 39, 49, 6, 46, 30},
  {'?', 294, 125, 47, 57, 6, 51, 38},
  {'@', 511, 182, 53, 53, 6, 47, 44},
  {'A', 596, 68, 54, 57, 6, 51, 45},
  {'B', 341, 125, 47, 57, 6, 51, 38},
  {'C', 534, 0, 51, 58, 6, 51, 42},
  {'D', 486, 68, 55, 57, 6, 51, 46},
  {'E', 102, 125, 48, 57, 6, 51, 39},
  {'F', 150, 125, 48, 57, 6, 51, 39},
  {'G', 916, 68, 51, 57, 6, 51, 43},
  {'H', 258, 68, 57, 57, 6, 51, 48},
  {'I', 915, 0, 26, 58, 4, 51, 18},
  {'J', 709, 125, 39, 57, 6, 51, 30},
  {'K', 529, 125, 46, 57, 6, 51, 37},
  {'L', 748, 125, 39, 57, 6, 51, 32},
  {'M', 142, 68, 58, 57, 6, 51, 49},
  {'N', 967, 68, 51, 57, 6, 51, 42},
  {'O', 186, 0, 59, 58, 6, 52, 50},
  {'P', 685, 0, 47, 58, 6, 51, 38},
  {'Q', 0, 0, 59, 68, 6, 51, 50},
  {'R', 388, 125, 47, 57, 6, 51, 38},
  {'S', 732, 0, 47, 58, 6, 51, 38},
  {'T', 812, 68, 52, 57, 6, 51, 43},
  {'U', 315, 68, 57, 57, 6, 51, 48},
  {'V', 245, 0, 59, 58, 6, 52, 49},
  {'W', 0, 68, 71, 57, 6, 51, 61},
  {'X', 650, 68, 54, 57, 6, 51, 46},
  {'Y', 422, 0, 56, 58, 6, 51, 47},
  {'Z', 621, 125, 44, 57, 6, 51, 38},
  {'[', 892, 125, 25, 57, 6, 51, 16},
  {'\\', 441, 182, 36, 54, 6, 50, 27},
  {']', 917, 125, 25, 57, 6, 51, 16},
  {'^', 818, 182, 43, 34, 6, 54, 34},
  {'_', 102, 238, 44, 18, 6, 3, 35},
  {'`', 47, 238, 23, 21, 6, 54, 10},
  {'a', 704, 68, 54, 57, 6, 51, 45},
  {'b', 435, 125, 47, 57, 6, 51, 38},
  {'c', 585, 0, 51, 58, 6, 51, 42},
  {'d', 541, 68, 55, 57, 6, 51, 46},
  {'e', 198, 125, 48, 57, 6, 51, 39},
  {'f', 246, 125, 48, 57, 6, 51, 39},
  {'g', 0, 125, 51, 57, 6, 51, 43},
  {'h', 372, 68, 57, 57, 6, 51, 48},
  {'i', 941, 0, 26, 58, 4, 51, 21},
  {'j', 787, 125, 39, 57, 6, 51, 30},
  {'k', 575, 125, 46, 57, 6, 51, 37},
  {'l', 826, 125, 39, 57, 6, 51, 30},
  {'m', 200, 68, 58, 57, 6, 51, 49},
  {'n', 51, 125, 51, 57, 6, 51, 42},
  {'o', 304, 0, 59, 58, 6, 52, 50},
  {'p', 779, 0, 47, 58, 6, 51, 38},
  {'q', 59, 0, 59, 68, 6, 51, 50},
  {'r', 482, 125, 47, 57, 6, 51, 38},
  {'s', 826, 0, 47, 58, 6, 51, 38},
  {'t', 864, 68, 52, 57, 6, 51, 43},
  {'u', 429, 68, 57, 57, 6, 51, 48},
  {'v', 363, 0, 59, 58, 6, 52, 49},
  {'w', 71, 68, 71, 57, 6, 51, 61},
  {'x', 758, 68, 54, 57, 6, 51, 46},
  {'y', 478, 0, 56, 58, 6, 51, 47},
  {'z', 665, 125, 44, 57, 6, 51, 38},
  {'{', 40, 182, 27, 56, 6, 51, 18},
  {'|', 942, 125, 19, 57, 0, 51, 16},
  {'}', 865, 125, 27, 57, 6, 51, 18},
  {'~', 934, 182, 44, 26, 6, 35, 34},
};

static Font font_TF2_Build = {"TF2 Build", 64, 0, 0, 1024, 512, 95, characters_TF2_Build};

static Character characters_TF2[] = {
  {' ', 423, 175, 12, 12, 6, 6, 22},
  {'!', 478, 118, 20, 55, 4, 51, 11},
  {'"', 313, 175, 18, 19, 6, 53, 8},
  {'#', 104, 175, 29, 29, 6, 38, 20},
  {'$', 0, 0, 61, 61, 6, 54, 49},
  {'%', 913, 118, 33, 47, 6, 46, 24},
  {'&', 946, 118, 36, 44, 4, 44, 30},
  {'\'', 268, 175, 17, 20, 6, 53, 8},
  {'(', 884, 61, 28, 57, 5, 51, 19},
  {')', 912, 61, 28, 57, 6, 51, 18},
  {'*', 155, 175, 26, 27, 6, 54, 17},
  {'+', 76, 175, 28, 31, 6, 39, 19},
  {',', 225, 175, 17, 21, 5, 11, 8},
  {'-', 331, 175, 26, 17, 6, 30, 18},
  {'.', 357, 175, 17, 17, 5, 13, 9},
  {'/', 607, 118, 31, 51, 6, 49, 22},
  {'0', 638, 118, 43, 50, 6, 47, 34},
  {'1', 888, 118, 25, 49, 6, 47, 16},
  {'2', 681, 118, 41, 50, 6, 48, 32},
  {'3', 762, 118, 39, 50, 6, 47, 30},
  {'4', 722, 118, 40, 50, 6, 48, 31},
  {'5', 837, 118, 35, 50, 6, 48, 26},
  {'6', 538, 118, 36, 51, 6, 49, 27},
  {'7', 801, 118, 36, 50, 6, 47, 27},
  {'8', 498, 118, 40, 51, 5, 48, 31},
  {'9', 574, 118, 33, 51, 6, 48, 24},
  {':', 57, 175, 19, 36, 6, 39, 10},
  {';', 39, 175, 18, 39, 6, 38, 9},
  {'<', 181, 175, 21, 27, 6, 36, 12},
  {'=', 285, 175, 28, 19, 6, 32, 19},
  {'>', 133, 175, 22, 28, 6, 36, 13},
  {'?', 435, 118, 43, 55, 6, 49, 34},
  {'@', 0, 175, 39, 39, 6, 38, 30},
  {'A', 817, 0, 43, 57, 6, 51, 32},
  {'B', 510, 61, 41, 57, 6, 51, 31},
  {'C', 242, 118, 42, 56, 5, 50, 33},
  {'D', 150, 118, 46, 56, 6, 50, 36},
  {'E', 258, 61, 42, 57, 6, 51, 32},
  {'F', 300, 61, 42, 57, 6, 51, 32},
  {'G', 860, 0, 43, 57, 5, 51, 35},
  {'H', 342, 61, 42, 57, 6, 51, 33},
  {'I', 965, 61, 23, 57, 6, 51, 14},
  {'J', 822, 61, 31, 57, 6, 51, 23},
  {'K', 903, 0, 43, 57, 6, 51, 32},
  {'L', 551, 61, 41, 57, 6, 51, 31},
  {'M', 359, 0, 48, 57, 6, 51, 39},
  {'N', 946, 0, 43, 57, 6, 51, 34},
  {'O', 161, 0, 50, 57, 5, 51, 41},
  {'P', 674, 61, 40, 57, 6, 51, 31},
  {'Q', 61, 0, 50, 61, 6, 51, 40},
  {'R', 261, 0, 49, 57, 6, 51, 38},
  {'S', 754, 61, 34, 57, 6, 51, 24},
  {'T', 639, 0, 45, 57, 6, 51, 35},
  {'U', 0, 61, 43, 57, 6, 51, 34},
  {'V', 729, 0, 44, 57, 6, 51, 34},
  {'W', 44, 118, 53, 56, 6, 50, 43},
  {'X', 455, 0, 46, 57, 6, 51, 36},
  {'Y', 501, 0, 46, 57, 6, 51, 35},
  {'Z', 284, 118, 42, 56, 6, 50, 32},
  {'[', 0, 118, 22, 57, 6, 51, 11},
  {'\\', 982, 118, 28, 44, 5, 42, 18},
  {']', 22, 118, 22, 57, 6, 51, 12},
  {'^', 202, 175, 23, 22, 6, 48, 14},
  {'_', 391, 175, 32, 16, 6, 6, 23},
  {'`', 374, 175, 17, 17, 3, 56, 9},
  {'a', 43, 61, 43, 57, 6, 51, 32},
  {'b', 592, 61, 41, 57, 6, 51, 31},
  {'c', 326, 118, 42, 56, 5, 50, 33},
  {'d', 196, 118, 46, 56, 6, 50, 36},
  {'e', 384, 61, 42, 57, 6, 51, 32},
  {'f', 426, 61, 42, 57, 6, 51, 32},
  {'g', 86, 61, 43, 57, 5, 51, 35},
  {'h', 468, 61, 42, 57, 6, 51, 33},
  {'i', 988, 61, 23, 57, 6, 51, 14},
  {'j', 853, 61, 31, 57, 6, 51, 23},
  {'k', 129, 61, 43, 57, 6, 51, 32},
  {'l', 633, 61, 41, 57, 6, 51, 31},
  {'m', 407, 0, 48, 57, 6, 51, 39},
  {'n', 172, 61, 43, 57, 6, 51, 34},
  {'o', 211, 0, 50, 57, 5, 51, 41},
  {'p', 714, 61, 40, 57, 6, 51, 31},
  {'q', 111, 0, 50, 61, 6, 51, 40},
  {'r', 310, 0, 49, 57, 6, 51, 38},
  {'s', 788, 61, 34, 57, 6, 51, 24},
  {'t', 684, 0, 45, 57, 6, 51, 35},
  {'u', 215, 61, 43, 57, 6, 51, 34},
  {'v', 773, 0, 44, 57, 6, 51, 34},
  {'w', 97, 118, 53, 56, 6, 50, 43},
  {'x', 547, 0, 46, 57, 6, 51, 36},
  {'y', 593, 0, 46, 57, 6, 51, 35},
  {'z', 368, 118, 42, 56, 6, 50, 32},
  {'{', 940, 61, 25, 57, 6, 51, 16},
  {'|', 872, 118, 16, 50, 4, 48, 9},
  {'}', 410, 118, 25, 56, 6, 50, 16},
  {'~', 242, 175, 26, 20, 6, 30, 17},
};
static Font font_TF2 = {"TF2", 64, 0, 0, 1024, 256, 95, characters_TF2};

static Character characters_LiberationSans[] = {
  {' ', 388, 247, 12, 12, 6, 6, 18},
  {'!', 68, 189, 19, 58, 0, 52, 18},
  {'"', 156, 247, 29, 28, 3, 52, 22},
  {'#', 705, 0, 46, 60, 5, 53, 35},
  {'$', 280, 0, 43, 69, 4, 56, 35},
  {'%', 380, 0, 61, 61, 2, 53, 57},
  {'&', 605, 0, 51, 60, 3, 53, 42},
  {'\'', 185, 247, 19, 28, 3, 52, 12},
  {'(', 161, 0, 27, 72, 2, 53, 21},
  {')', 133, 0, 28, 72, 2, 53, 21},
  {'*', 81, 247, 33, 32, 4, 53, 25},
  {'+', 769, 189, 42, 43, 2, 44, 37},
  {',', 204, 247, 19, 27, 1, 12, 18},
  {'-', 291, 247, 29, 18, 4, 25, 21},
  {'.', 320, 247, 19, 18, 0, 12, 18},
  {'/', 751, 0, 30, 60, 6, 53, 18},
  {'0', 43, 72, 42, 59, 3, 52, 35},
  {'1', 39, 189, 29, 58, -1, 52, 35},
  {'2', 582, 131, 43, 58, 4, 52, 35},
  {'3', 85, 72, 42, 59, 3, 52, 35},
  {'4', 538, 131, 44, 58, 5, 52, 35},
  {'5', 625, 131, 43, 58, 3, 51, 35},
  {'6', 0, 72, 43, 59, 4, 52, 35},
  {'7', 711, 131, 42, 58, 3, 51, 35},
  {'8', 127, 72, 42, 59, 3, 52, 35},
  {'9', 169, 72, 42, 59, 3, 52, 35},
  {':', 750, 189, 19, 45, 0, 39, 18},
  {';', 170, 189, 19, 54, 1, 39, 18},
  {'<', 811, 189, 42, 43, 2, 44, 37},
  {'=', 114, 247, 42, 31, 2, 38, 37},
  {'>', 0, 247, 42, 43, 2, 44, 37},
  {'?', 211, 72, 42, 59, 3, 53, 35},
  {'@', 0, 0, 72, 72, 3, 53, 65},
  {'A', 696, 72, 55, 58, 6, 52, 42},
  {'B', 398, 131, 47, 58, 1, 52, 42},
  {'C', 552, 0, 53, 60, 3, 53, 46},
  {'D', 106, 131, 50, 58, 1, 52, 46},
  {'E', 492, 131, 46, 58, 1, 52, 42},
  {'F', 668, 131, 43, 58, 1, 52, 39},
  {'G', 497, 0, 55, 60, 3, 53, 50},
  {'H', 206, 131, 48, 58, 1, 52, 46},
  {'I', 87, 189, 19, 58, 0, 52, 18},
  {'J', 500, 72, 38, 59, 4, 52, 32},
  {'K', 156, 131, 50, 58, 1, 52, 42},
  {'L', 753, 131, 41, 58, 1, 52, 35},
  {'M', 640, 72, 56, 58, 1, 52, 53},
  {'N', 254, 131, 48, 58, 1, 52, 46},
  {'O', 441, 0, 56, 60, 3, 53, 50},
  {'P', 445, 131, 47, 58, 1, 52, 42},
  {'Q', 323, 0, 57, 62, 3, 53, 50},
  {'R', 54, 131, 52, 58, 1, 52, 46},
  {'S', 656, 0, 49, 60, 3, 53, 42},
  {'T', 302, 131, 48, 58, 4, 52, 39},
  {'U', 811, 0, 48, 59, 1, 52, 46},
  {'V', 751, 72, 54, 58, 6, 52, 42},
  {'W', 569, 72, 71, 58, 5, 52, 60},
  {'X', 805, 72, 54, 58, 6, 52, 42},
  {'Y', 0, 131, 54, 58, 6, 52, 42},
  {'Z', 350, 131, 48, 58, 5, 52, 39},
  {'[', 205, 0, 25, 71, 2, 52, 18},
  {'\\', 781, 0, 30, 60, 6, 53, 18},
  {']', 230, 0, 25, 71, 5, 52, 18},
  {'^', 42, 247, 39, 37, 4, 53, 30},
  {'_', 339, 247, 49, 16, 7, -3, 35},
  {'`', 267, 247, 24, 21, 3, 52, 21},
  {'a', 189, 189, 43, 47, 4, 40, 35},
  {'b', 295, 72, 41, 59, 2, 52, 35},
  {'c', 318, 189, 41, 47, 3, 40, 32},
  {'d', 336, 72, 41, 59, 4, 52, 35},
  {'e', 232, 189, 43, 47, 4, 40, 35},
  {'f', 538, 72, 31, 59, 5, 53, 18},
  {'g', 377, 72, 41, 59, 4, 40, 35},
  {'h', 0, 189, 39, 58, 2, 52, 35},
  {'i', 106, 189, 18, 58, 2, 52, 14},
  {'j', 255, 0, 25, 71, 9, 52, 14},
  {'k', 794, 131, 40, 58, 2, 52, 32},
  {'l', 124, 189, 18, 58, 2, 52, 14},
  {'m', 399, 189, 57, 46, 2, 40, 53},
  {'n', 456, 189, 39, 46, 2, 40, 35},
  {'o', 275, 189, 43, 47, 4, 40, 35},
  {'p', 418, 72, 41, 59, 2, 40, 35},
  {'q', 459, 72, 41, 59, 4, 40, 35},
  {'r', 534, 189, 30, 46, 2, 40, 21},
  {'s', 359, 189, 40, 47, 4, 40, 32},
  {'t', 142, 189, 28, 57, 5, 51, 18},
  {'u', 495, 189, 39, 46, 2, 39, 35},
  {'v', 622, 189, 43, 45, 5, 39, 32},
  {'w', 564, 189, 58, 45, 6, 39, 46},
  {'x', 665, 189, 43, 45, 5, 39, 32},
  {'y', 253, 72, 42, 59, 5, 39, 32},
  {'z', 708, 189, 42, 45, 5, 39, 32},
  {'{', 103, 0, 30, 72, 4, 53, 21},
  {'|', 188, 0, 17, 72, 0, 53, 16},
  {'}', 72, 0, 31, 72, 5, 53, 21},
  {'~', 223, 247, 44, 23, 3, 34, 37},
};
static Font font_LiberationSans = {"Liberation Sans", 64, 0, 0, 859, 290, 95, characters_LiberationSans};

static Character characters_TF2_Professor[] = {
  {' ', 1011, 251, 12, 12, 6, 6, 27},
  {'!', 886, 104, 18, 71, 2, 71, 14},
  {'"', 734, 251, 26, 32, 2, 90, 24},
  {'#', 550, 182, 61, 53, 2, 67, 58},
  {'$', 171, 0, 43, 94, 2, 88, 40},
  {'%', 206, 182, 70, 66, 2, 80, 66},
  {'&', 49, 104, 51, 77, 3, 82, 46},
  {'\'', 805, 251, 24, 30, 2, 84, 20},
  {'(', 135, 0, 36, 96, 2, 88, 32},
  {')', 214, 0, 38, 90, 2, 84, 34},
  {'*', 633, 251, 35, 37, 2, 91, 32},
  {'+', 410, 182, 59, 55, 2, 71, 56},
  {',', 668, 251, 26, 36, 2, 20, 22},
  {'-', 898, 251, 37, 20, 2, 44, 34},
  {'.', 857, 251, 21, 24, 2, 20, 16},
  {'/', 377, 182, 33, 57, 1, 58, 30},
  {'0', 276, 182, 54, 64, 2, 59, 50},
  {'1', 561, 104, 41, 72, 2, 64, 38},
  {'2', 0, 182, 49, 69, 2, 63, 46},
  {'3', 170, 182, 36, 68, 2, 60, 32},
  {'4', 842, 104, 44, 71, 2, 65, 40},
  {'5', 516, 104, 45, 72, 2, 66, 42},
  {'6', 642, 104, 37, 72, 2, 64, 34},
  {'7', 793, 104, 49, 71, 2, 63, 44},
  {'8', 602, 104, 40, 72, 3, 66, 34},
  {'9', 956, 104, 41, 70, 1, 57, 38},
  {':', 53, 251, 22, 50, 2, 54, 18},
  {';', 839, 182, 28, 52, 2, 56, 24},
  {'<', 521, 251, 38, 46, 2, 68, 34},
  {'=', 760, 251, 45, 30, 2, 56, 42},
  {'>', 469, 182, 41, 54, 2, 70, 38},
  {'?', 344, 0, 48, 84, 2, 78, 40},
  {'@', 71, 182, 58, 68, 2, 66, 54},
  {'A', 918, 0, 48, 79, 2, 76, 43},
  {'B', 343, 104, 48, 74, 2, 78, 44},
  {'C', 148, 104, 45, 77, 3, 72, 40},
  {'D', 248, 104, 48, 75, 2, 75, 44},
  {'E', 100, 104, 48, 77, 4, 77, 42},
  {'F', 129, 182, 41, 68, 3, 70, 36},
  {'G', 502, 0, 46, 82, 3, 76, 42},
  {'H', 193, 104, 55, 76, 3, 74, 50},
  {'I', 49, 182, 22, 69, 2, 73, 18},
  {'J', 966, 0, 41, 79, 2, 75, 38},
  {'K', 777, 0, 53, 80, 3, 76, 48},
  {'L', 0, 104, 49, 78, 3, 77, 44},
  {'M', 679, 104, 63, 71, 3, 69, 58},
  {'N', 904, 104, 52, 70, 2, 72, 48},
  {'O', 742, 104, 51, 71, 6, 74, 44},
  {'P', 391, 104, 39, 74, 3, 77, 36},
  {'Q', 285, 0, 59, 84, 3, 84, 54},
  {'R', 614, 0, 57, 81, 2, 82, 51},
  {'S', 477, 104, 39, 73, 2, 74, 34},
  {'T', 548, 0, 66, 81, 5, 80, 56},
  {'U', 430, 104, 47, 73, 3, 77, 41},
  {'V', 453, 0, 49, 83, 2, 81, 44},
  {'W', 851, 0, 67, 79, 2, 76, 64},
  {'X', 296, 104, 47, 75, 2, 74, 44},
  {'Y', 671, 0, 50, 81, 2, 82, 46},
  {'Z', 721, 0, 56, 80, 2, 80, 52},
  {'[', 252, 0, 33, 87, 2, 85, 30},
  {'\\', 392, 0, 61, 83, 2, 84, 56},
  {']', 0, 0, 44, 104, 2, 92, 40},
  {'^', 829, 251, 28, 28, 2, 78, 24},
  {'_', 935, 251, 76, 19, 2, 20, 72},
  {'`', 878, 251, 20, 24, 2, 89, 22},
  {'a', 510, 182, 40, 54, 2, 48, 34},
  {'b', 611, 182, 35, 53, 2, 48, 31},
  {'c', 941, 182, 32, 51, 4, 47, 26},
  {'d', 120, 251, 35, 49, 1, 47, 32},
  {'e', 905, 182, 36, 51, 2, 47, 32},
  {'f', 646, 182, 31, 53, 2, 50, 28},
  {'g', 867, 182, 38, 51, 2, 47, 33},
  {'h', 722, 182, 42, 52, 2, 48, 37},
  {'i', 155, 251, 17, 49, 1, 47, 14},
  {'j', 294, 251, 30, 48, 2, 46, 26},
  {'k', 677, 182, 45, 52, 2, 46, 39},
  {'l', 257, 251, 37, 48, 0, 47, 34},
  {'m', 430, 251, 51, 46, 2, 42, 46},
  {'n', 75, 251, 45, 49, 2, 47, 40},
  {'o', 481, 251, 40, 46, 3, 43, 35},
  {'p', 803, 182, 36, 52, 2, 48, 31},
  {'q', 559, 251, 42, 45, 2, 43, 35},
  {'r', 219, 251, 38, 48, 0, 44, 33},
  {'s', 324, 251, 29, 48, 3, 45, 24},
  {'t', 172, 251, 47, 48, 5, 44, 38},
  {'u', 399, 251, 31, 47, 2, 43, 27},
  {'v', 601, 251, 32, 43, 2, 41, 26},
  {'w', 0, 251, 53, 50, 4, 46, 45},
  {'x', 353, 251, 46, 47, 5, 45, 35},
  {'y', 330, 182, 47, 57, 5, 51, 38},
  {'z', 764, 182, 39, 52, 3, 48, 32},
  {'{', 44, 0, 47, 98, 2, 92, 44},
  {'|', 830, 0, 21, 80, 2, 80, 18},
  {'}', 91, 0, 44, 98, 5, 93, 34},
  {'~', 694, 251, 40, 33, 2, 59, 36},
};
static Font font_TF2_Professor = {"TF2 Professor", 128, 0, 0, 1024, 512, 95, characters_TF2_Professor};

static Character characters_Roboto_Mono[] = {
  {' ', 167, 352, 12, 12, 6, 6, 77},
  {'!', 675, 144, 27, 104, -24, 97, 77},
  {'"', 1859, 249, 44, 42, -16, 102, 77},
  {'#', 702, 144, 82, 103, 2, 97, 77},
  {'$', 324, 0, 70, 131, -4, 112, 77},
  {'%', 1322, 0, 83, 106, 3, 98, 77},
  {'&', 1405, 0, 80, 106, -1, 98, 77},
  {'\'', 1903, 249, 22, 42, -25, 102, 77},
  {'(', 0, 0, 45, 144, -16, 109, 77},
  {')', 45, 0, 45, 144, -14, 109, 77},
  {'*', 1497, 249, 72, 73, -4, 97, 77},
  {'+', 1422, 249, 75, 78, -1, 81, 77},
  {',', 1829, 249, 30, 47, -16, 20, 77},
  {'-', 37, 352, 60, 22, -8, 51, 77},
  {'.', 2008, 249, 30, 30, -25, 22, 77},
  {'/', 568, 0, 60, 111, -10, 97, 77},
  {'0', 1788, 0, 71, 106, -3, 98, 77},
  {'1', 342, 249, 47, 103, -7, 97, 77},
  {'2', 0, 144, 74, 105, 1, 98, 77},
  {'3', 1859, 0, 70, 106, 0, 98, 77},
  {'4', 1263, 144, 78, 103, 1, 97, 77},
  {'5', 572, 144, 69, 104, -6, 97, 77},
  {'6', 74, 144, 70, 105, -3, 97, 77},
  {'7', 1491, 144, 73, 103, -1, 97, 77},
  {'8', 1929, 0, 70, 106, -5, 98, 77},
  {'9', 144, 144, 70, 105, -3, 98, 77},
  {':', 459, 249, 30, 85, -28, 77, 77},
  {';', 641, 144, 34, 104, -24, 77, 77},
  {'<', 1636, 249, 65, 69, -5, 75, 77},
  {'=', 1761, 249, 68, 48, -5, 65, 77},
  {'>', 1569, 249, 67, 69, -5, 75, 77},
  {'?', 283, 144, 66, 105, -6, 98, 77},
  {'@', 349, 144, 80, 104, 2, 97, 77},
  {'A', 865, 144, 80, 103, 1, 97, 77},
  {'B', 1852, 144, 71, 103, -5, 97, 77},
  {'C', 1713, 0, 75, 106, -1, 98, 77},
  {'D', 1417, 144, 74, 103, -4, 97, 77},
  {'E', 71, 249, 68, 103, -5, 97, 77},
  {'F', 139, 249, 68, 103, -6, 97, 77},
  {'G', 1485, 0, 76, 106, 0, 98, 77},
  {'H', 1923, 144, 71, 103, -3, 97, 77},
  {'I', 275, 249, 67, 103, -5, 97, 77},
  {'J', 501, 144, 71, 104, 0, 97, 77},
  {'K', 1341, 144, 76, 103, -5, 97, 77},
  {'L', 207, 249, 68, 103, -6, 97, 77},
  {'M', 1564, 144, 72, 103, -3, 97, 77},
  {'N', 0, 249, 71, 103, -3, 97, 77},
  {'O', 1561, 0, 76, 106, -1, 98, 77},
  {'P', 1636, 144, 72, 103, -6, 97, 77},
  {'Q', 416, 0, 79, 120, 0, 98, 77},
  {'R', 1708, 144, 72, 103, -5, 97, 77},
  {'S', 1637, 0, 76, 106, -1, 98, 77},
  {'T', 945, 144, 80, 103, 1, 97, 77},
  {'U', 429, 144, 72, 104, -3, 97, 77},
  {'V', 1105, 144, 79, 103, 1, 97, 77},
  {'W', 784, 144, 81, 103, 1, 97, 77},
  {'X', 1184, 144, 79, 103, 0, 97, 77},
  {'Y', 1025, 144, 80, 103, 2, 97, 77},
  {'Z', 1780, 144, 72, 103, -1, 97, 77},
  {'[', 90, 0, 37, 136, -21, 110, 77},
  {'\\', 628, 0, 60, 111, -9, 97, 77},
  {']', 127, 0, 37, 136, -19, 110, 77},
  {'^', 1701, 249, 60, 61, -9, 97, 77},
  {'_', 97, 352, 70, 21, -4, 6, 77},
  {'`', 0, 352, 37, 29, -20, 99, 77},
  {'a', 706, 249, 70, 82, -4, 75, 77},
  {'b', 688, 0, 70, 109, -5, 102, 77},
  {'c', 635, 249, 71, 82, -3, 75, 77},
  {'d', 758, 0, 69, 109, -3, 102, 77},
  {'e', 563, 249, 72, 82, -2, 75, 77},
  {'f', 495, 0, 73, 111, -4, 105, 77},
  {'g', 898, 0, 69, 108, -3, 75, 77},
  {'h', 1036, 0, 68, 108, -5, 102, 77},
  {'i', 214, 144, 69, 105, -7, 98, 77},
  {'j', 272, 0, 52, 132, -7, 98, 77},
  {'k', 827, 0, 71, 108, -5, 102, 77},
  {'l', 967, 0, 69, 108, -7, 102, 77},
  {'m', 845, 249, 78, 81, 0, 75, 77},
  {'n', 923, 249, 68, 81, -5, 75, 77},
  {'o', 489, 249, 74, 82, -2, 75, 77},
  {'p', 1184, 0, 69, 107, -5, 75, 77},
  {'q', 1253, 0, 69, 107, -3, 75, 77},
  {'r', 1058, 249, 59, 81, -15, 75, 77},
  {'s', 776, 249, 69, 82, -5, 75, 77},
  {'t', 389, 249, 70, 97, -3, 90, 77},
  {'u', 991, 249, 67, 81, -5, 74, 77},
  {'v', 1200, 249, 76, 80, 0, 74, 77},
  {'w', 1117, 249, 83, 80, 3, 74, 77},
  {'x', 1276, 249, 76, 80, -1, 74, 77},
  {'y', 1104, 0, 80, 107, 2, 74, 77},
  {'z', 1352, 249, 70, 80, -4, 74, 77},
  {'{', 164, 0, 54, 135, -14, 106, 77},
  {'|', 394, 0, 22, 128, -28, 97, 77},
  {'}', 218, 0, 54, 135, -14, 106, 77},
  {'~', 1925, 249, 83, 37, 3, 56, 77},
};

static Font font_Roboto_Mono = {"Roboto Mono", 128, 0, 0, 2048, 512, 95, characters_Roboto_Mono};

struct MapFont
{
	Font *font;
	const char *pszMaterial;
};
MapFont g_mapfonts[]
{
	{ &font_TF2_Build, "editor/worldtext_1" },
	{ &font_TF2_Build, "editor/worldtext_2" },
	{ &font_TF2, "editor/worldtext_3" },
	{ &font_TF2, "editor/worldtext_4" },
	{ &font_LiberationSans, "editor/worldtext_5" },
	{ &font_LiberationSans, "editor/worldtext_6" },
	{ &font_TF2_Professor, "editor/worldtext_7" },
	{ &font_TF2_Professor, "editor/worldtext_8" },
	{ &font_Roboto_Mono, "editor/worldtext_9" },
	{ &font_Roboto_Mono, "editor/worldtext_10" },
	{ &font_Roboto_Mono, "editor/worldtext_11" },
	{ &font_Roboto_Mono, "editor/worldtext_12" },
	{ &font_TF2_Build, "editor/worldtext_13" },
};

typedef struct {
    double r;
    double g;
    double b;
} rgb;

typedef struct {
    double h;
    double s;
    double v;
} hsv;

rgb hsv2rgb(hsv HSV)
{
    rgb RGB;
    double H = HSV.h, S = HSV.s, V = HSV.v,
            P, Q, T,
            fract;
    (H == 360.)?(H = 0.):(H /= 60.);
    fract = H - floor(H);
    P = V*(1. - S);
    Q = V*(1. - S*fract);
    T = V*(1. - S*(1. - fract));
    if      (0. <= H && H < 1.)
        RGB = rgb{V, T, P};
    else if (1. <= H && H < 2.)
        RGB = rgb{Q, V, P};
    else if (2. <= H && H < 3.)
        RGB = rgb{P, V, T};
    else if (3. <= H && H < 4.)
        RGB = rgb{P, Q, V};
    else if (4. <= H && H < 5.)
        RGB = rgb{T, P, V};
    else if (5. <= H && H < 6.)
        RGB = rgb{V, P, Q};
    else
        RGB = rgb{0., 0., 0.};
    return RGB;
}

void RecvProxy_Text( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_PointWorldText* pWorldText = static_cast<C_PointWorldText*>( pStruct );
	pWorldText->SetText( pData->m_Value.m_pString );
}

void RecvProxy_Font( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_PointWorldText* pWorldText = static_cast<C_PointWorldText*>( pStruct );
	pWorldText->SetFont( pData->m_Value.m_Int );
}

IMPLEMENT_CLIENTCLASS_DT(C_PointWorldText, DT_PointWorldText, CPointWorldText)
	RecvPropString(RECVINFO( m_szText ), 0, RecvProxy_Text ),
	RecvPropFloat( RECVINFO( m_flTextSize ) ),
	RecvPropFloat( RECVINFO( m_flTextSpacingX ) ),
	RecvPropFloat( RECVINFO( m_flTextSpacingY ) ),
	RecvPropInt( RECVINFO( m_colTextColor ) ),
	RecvPropInt( RECVINFO( m_nOrientation ) ),
	RecvPropInt( RECVINFO( m_nFont ), 0, RecvProxy_Font ),
	RecvPropBool( RECVINFO( m_bRainbow ) ),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS(point_worldtext, C_PointWorldText);

C_PointWorldText::C_PointWorldText()
	: C_BaseEntity()
	, m_flTextSize( 10.0f )
	, m_flTextSpacingX( 0.0f )
	, m_flTextSpacingY( 0.0f )
	, m_nTextLength( 0 )
	, m_Font( nullptr )
{
	V_memset( m_szText, 0, sizeof( m_szText ) );
	m_colTextColor.r = 255;
	m_colTextColor.g = 255;
	m_colTextColor.b = 255;
	m_colTextColor.a = 255;
	m_nOrientation = 0;
	SetFont( 0 );
	m_bRainbow = false;
}

C_PointWorldText::~C_PointWorldText()
{
}

void C_PointWorldText::Spawn()
{
	BaseClass::Spawn();
}

bool C_PointWorldText::ShouldDraw()
{
	return !IsEffectActive( EF_NODRAW );
}

void C_PointWorldText::ClientThink()
{
	BaseClass::ClientThink();
	UpdateTextWorldSize();
	UpdateRenderBounds();
}

void C_PointWorldText::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );
	
	// Text might have changed. Update bounds.
	// Position queries during entity update are not valid, so we must do it during think instead
	SetNextClientThink( 0.0f );
}

void C_PointWorldText::ComputeCornerVertices( const QAngle &angles, const Vector &origin, Vector *pVerts ) const
{
	Vector ViewForward( 1.0f, 0.0f, 0.0f );
	Vector ViewUp( 0.0f, 1.0f, 0.0f );
	Vector ViewRight( 0.0f, 0.0f, -1.0f );
	AngleVectors( angles, &ViewForward, &ViewRight, &ViewUp );

	float flStrLength = (float)m_nTextLength;
	flStrLength = Max( flStrLength, 1.0f );

	pVerts[ 0 ] = origin - ( ViewRight + ViewUp );
	pVerts[ 1 ] = pVerts[ 0 ] + GetTextWorldWidth() * ViewRight;
	pVerts[ 2 ] = pVerts[ 1 ] + GetTextWorldHeight() * ViewUp;
	pVerts[ 3 ] = pVerts[ 0 ] + GetTextWorldHeight() * ViewUp;
}

void C_PointWorldText::UpdateRenderBounds()
{
	Assert( C_BaseEntity::IsAbsQueriesValid() );

	Vector cornerVerts[ 4 ];
	if ( m_nOrientation == 0 )
	{
		ComputeCornerVertices( QAngle(0.0f, 0.0f, 0.0f), vec3_origin, cornerVerts );
	}
	else
	{
		// centered origin and oriented sprite, just take aabb as bounds
		float flWidth = GetTextWorldWidth() * 0.5f;
		cornerVerts[0].Init( -flWidth, -flWidth, -flWidth );
		cornerVerts[1] = cornerVerts[0];
		cornerVerts[2].Init( flWidth, flWidth, flWidth );
		cornerVerts[3] = cornerVerts[2];
	}

	m_localBBMin = VectorMin( VectorMin( cornerVerts[ 0 ], cornerVerts[ 1 ] ), VectorMin( cornerVerts[ 2 ], cornerVerts[ 3 ] ) );
	m_localBBMax = VectorMax( VectorMax( cornerVerts[ 0 ], cornerVerts[ 1 ] ), VectorMax( cornerVerts[ 2 ], cornerVerts[ 3 ] ) );
}

void C_PointWorldText::GetRenderBounds( Vector& mins, Vector& maxs )
{
	mins = m_localBBMin;
	maxs = m_localBBMax;
}

// allow parenting text to players
bool C_PointWorldText::ValidateEntityAttachedToPlayer( bool &bShouldRetry )
{
	bShouldRetry = false;
	return true;
}

void C_PointWorldText::SetText( const char* pszText )
{
	m_nTextLength = V_strlen( pszText );
	V_strncpy( m_szText, pszText, sizeof(m_szText) );
	UpdateTextWorldSize();
}

void C_PointWorldText::SetFont( int nFont )
{
	m_nFont = Clamp<int>(nFont, 0, (int)(ARRAYSIZE(g_mapfonts) - 1));
	m_Font.Init( g_mapfonts[m_nFont].pszMaterial, TEXTURE_GROUP_OTHER, true);
	UpdateTextWorldSize();
}

bool C_PointWorldText::IsTransparent( void )
{
	return true;
}

void C_PointWorldText::UpdateTextWorldSize()
{
	CalcTextTotalSize( m_flTextWorldWidth, m_flTextWorldHeight );
}
void C_PointWorldText::CalcTextTotalSize(float &outWidth, float &outHeight)
{
	outWidth = 0.0f;
	outHeight = 0.0f;

	const char *szText = m_szText;
	if ( !szText[0] )
		return;

	int nNumChars = m_nTextLength;
	if ( !nNumChars )
		return;

	float screenSize = m_flTextSize;
	float screenSpacingX = GetTextSpacingX();
	float screenSpacingY = GetTextSpacingY();
	Font* font = g_mapfonts[m_nFont].font;
	outHeight += font->size;
	float flLineWidth = 0.0f;
	for ( int i = 0; i < nNumChars; i++ )
	{
		char nChar = *(szText++);
		unsigned int nCharIdx = Clamp<unsigned int>( ( unsigned int )( nChar ) - 32, 0u, ( unsigned int )( ARRAYSIZE( characters_TF2_Build ) - 1u ) );
		Character *character = &font->characters[ nCharIdx ];
		float scale = screenSize / (float)font->size;
		if ( nChar == '\n' )
		{
			outWidth = Max( outWidth, flLineWidth );
			flLineWidth = 0.0f;
			outHeight += (font->size + screenSpacingY) * scale;
			continue;
		}
		flLineWidth += (character->advance + screenSpacingX) * scale;
	}
	outWidth = Max( outWidth, flLineWidth );
}

float C_PointWorldText::GetTextWorldWidth() const
{
	return m_flTextWorldWidth;
}
float C_PointWorldText::GetTextWorldHeight() const
{
	return m_flTextWorldHeight;
}
float C_PointWorldText::GetTextSpacingX() const
{
	return m_flTextSpacingX;
}
float C_PointWorldText::GetTextSpacingY() const
{
	return m_flTextSpacingY;
}

ConVar pointworldtext_rainbowspeed_time( "pointworldtext_rainbowspeed_time", "100.0f" );
ConVar pointworldtext_rainbowspeed_char( "pointworldtext_rainbowspeed_char", "15.0f" );

int C_PointWorldText::DrawModel( int flags )
{
	if ( ( flags & STUDIO_SHADOWDEPTHTEXTURE ) )
		return 0;

	const char *szText = m_szText;
	if ( !szText[0] )
		return 0;

	int nNumChars = m_nTextLength;
	if ( !nNumChars )
		return 0;

	IMaterial* pDebugText = m_Font;
	if ( !pDebugText )
		return 0;

	Vector ViewForward( 1.0f, 0.0f, 0.0f );
	Vector ViewUp( 0.0f, 1.0f, 0.0f );
	Vector ViewRight( 0.0f, 0.0f, -1.0f );
	Vector vecStartPos;
	VectorCopy( GetAbsOrigin(), vecStartPos );

	float screenSize = m_flTextSize;
	float screenSpacingX = GetTextSpacingX();
	float screenSpacingY = GetTextSpacingY();

	switch ( m_nOrientation )
	{
		// always orient towards screen
		case 1:
			ViewForward = -CurrentViewForward();
			ViewUp = CurrentViewUp();
			ViewRight = CurrentViewRight();
			// center the text for nicer rotation
			vecStartPos -= GetTextWorldWidth() * 0.5f * ViewRight;
			break;
		// orient towards screen but align with Z axis
		case 2:
			ViewForward = -CurrentViewForward();
			ViewUp = Vector(0, 0, 1);
			ViewRight = CurrentViewRight();
			// center the text for nicer rotation
			vecStartPos -= GetTextWorldWidth() * 0.5f * ViewRight;
			break;
		// entity orientation
		default:
			AngleVectors( GetAbsAngles(), &ViewForward, &ViewRight, &ViewUp );
			break;
	}

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
	pRenderContext->Bind( pDebugText );

	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, nNumChars );

	Vector vecOrigStartPos = vecStartPos;

	Font *font = g_mapfonts[m_nFont].font;

	for ( int i = 0; i < nNumChars; i++ )
	{
		char nChar = *(szText++);
		unsigned int nCharIdx = Clamp( ( unsigned int )( nChar ) - 32, 0u, ( unsigned int )( ARRAYSIZE( characters_TF2_Build ) - 1u ) );
		Character *character = &font->characters[ nCharIdx ];
		float scale = screenSize / (float)font->size;
		if ( nChar == '\n' )
		{
			vecOrigStartPos -= ( ViewUp * ( (font->size + screenSpacingY) * scale ) );
			vecStartPos = vecOrigStartPos;
			continue;
		}
		color32 color = m_colTextColor;
		if ( m_bRainbow )
		{
			float h = gpGlobals->curtime * pointworldtext_rainbowspeed_time.GetFloat() + i * pointworldtext_rainbowspeed_char.GetFloat();
			h = fmodf( h, 360.0f );
			rgb value = hsv2rgb(hsv{ h, 0.7, 1.0 });
			color.r = value.r * 255.0f;
			color.g = value.g * 255.0f;
			color.b = value.b * 255.0f;
		}

		byte* pColor = (byte*)&color;
		if ( nChar != ' ' )
		{
			float x, y, s, t;

			x = -character->originX;
			y = -character->originY;
			s = character->x / (float)font->width;
			t = character->y / (float)font->height;
			Vector v0 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v0.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();

			x = -character->originX;
			y = -character->originY + character->height;
			s = character->x / (float)font->width;
			t = (character->y + character->height) / (float)font->height;
			Vector v2 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v2.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();

			x = -character->originX + character->width;
			y = -character->originY + character->height;
			s = (character->x + character->width) / (float)font->width;
			t = (character->y + character->height) / (float)font->height;
			Vector v3 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v3.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();

			x = -character->originX + character->width;
			y = -character->originY;
			s = (character->x + character->width) / (float)font->width;
			t = (character->y) / (float)font->height;
			Vector v1 = vecStartPos + ViewRight * x * scale + ViewUp * (- y) * scale;
			meshBuilder.Position3fv( v1.Base() );
			meshBuilder.TexCoord2f( 0, s, t );
			meshBuilder.Color4ubv( pColor );
			meshBuilder.AdvanceVertex();
		}
		vecStartPos += ViewRight * ((character->advance + screenSpacingX) * scale);
	}
	meshBuilder.End();
	pMesh->Draw();
	return 1;
}