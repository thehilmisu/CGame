#include <math.h>
#include <stdlib.h>

// NoisePermutation structure (for 2D Perlin noise)
typedef struct {
    int perm[256];
} NoisePermutation;

// 2D gradient vectors (from original)
static const float gradients[4][2] = {
    {1.0f, 0.0f},
    {-1.0f, 0.0f},
    {0.0f, 1.0f},
    {0.0f, -1.0f}
};

// Dot product of gradient and distance vector
static float dot_gradient_2d(int grid_x, int grid_y, float x, float y, const NoisePermutation* perm) {
    // Hash function (from original - bit magic to reduce periodicity)
    unsigned int a = (unsigned int)grid_x;
    unsigned int b = (unsigned int)grid_y;
    
    const unsigned int w = 8 * sizeof(unsigned int);
    const unsigned int s = w / 2;
    
    a *= 3284157443U;
    b ^= a << s | a >> (w - s);
    b *= 1911520717U;
    a ^= b << s | b >> (w - s);
    a *= 2048419325U;
    
    // Get permutation index (with bounds checking)
    unsigned int idx1 = (a % 256);
    unsigned int idx2 = ((unsigned int)perm->perm[idx1] + b) % 256;
    unsigned int idx3 = (unsigned int)perm->perm[idx2] % 256;
    int index = perm->perm[idx3];
    
    // Get gradient vector
    const float* grad = gradients[index % 4];
    
    // Distance vector
    float dx = x - (float)grid_x;
    float dy = y - (float)grid_y;
    
    // Dot product
    return grad[0] * dx + grad[1] * dy;
}

// Smoothstep interpolation
static float interpolate_smooth(float a, float b, float x) {
    // Smoothstep: (b - a) * (3.0 - x * 2.0) * x * x + a
    return (b - a) * (3.0f - x * 2.0f) * x * x + a;
}

// 2D Perlin noise (matching original exactly)
float noise_perlin_2d(float x, float y, const NoisePermutation* perm) {
    int left_x = (int)floorf(x);
    int lower_y = (int)floorf(y);
    int right_x = left_x + 1;
    int upper_y = lower_y + 1;
    
    float lower_left = dot_gradient_2d(left_x, lower_y, x, y, perm);
    float lower_right = dot_gradient_2d(right_x, lower_y, x, y, perm);
    float upper_left = dot_gradient_2d(left_x, upper_y, x, y, perm);
    float upper_right = dot_gradient_2d(right_x, upper_y, x, y, perm);
    
    float lerped_lower = interpolate_smooth(lower_left, lower_right, x - left_x);
    float lerped_upper = interpolate_smooth(upper_left, upper_right, x - left_x);
    
    return interpolate_smooth(lerped_lower, lerped_upper, y - lower_y);
}
