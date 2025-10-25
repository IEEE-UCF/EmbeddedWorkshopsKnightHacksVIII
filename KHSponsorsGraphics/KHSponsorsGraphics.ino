#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include "float.h"
extern "C" {
  #include "ieeeucfsymbol.c"
}

// Define pins for display interface. You'll probably need to edit this for
// your own needs:
#define TFT_DC  17
#define TFT_CS 5
#define TFT_RESET 16

// Display constructor for primary hardware SPI connection -- the specific
// pins used for writing to the display are unique to each board and are not
// negotiable. "Soft" SPI (using any pins) is an option but performance is
// reduced; it's rarely used, see header file for syntax if needed.
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RESET);



void setup() {
  Serial.begin(9600);

  tft.begin(80000000);

  Serial.println("Setup done!");
  tft.fillScreen(GC9A01A_BLACK);
}

#define buffer_width 220
#define buffer_height 220
uint16_t buffer[buffer_width*buffer_height];
uint16_t rotAngle = 0;
float scale = 1;
bool direction = 1;
const uint16_t* image_pointers[18] = {ieeeucfsymbol, KH2025Small, GeminiSmall, OneEthosSmall, PheratechSmall, LMSmall, GitHubLogo, NVidiaSmall, NasaSmall, AMDSmall, StatSIGSmall, NextEraSmall, BNYSmall, ServiceNowSmall, AuritasSmall, MorganSmall, ImpressInkSmall, ShiniesSmall};
uint8_t image_switch = 0;
uint16_t roll = 0;

void loop(void) {
  rotateImage3D_Inverse(image_pointers[image_switch], buffer, IEEEUCFSYMBOL_WIDTH, IEEEUCFSYMBOL_HEIGHT, buffer_width, buffer_height, 0, rotAngle, roll, scale, 40);
  tft.drawRGBBitmap(240/2 - buffer_width/2, 240/2 - buffer_height/2, buffer, buffer_width, buffer_height);
  rotAngle = (rotAngle + 8) % 360;
  scale += direction ? 0.04 : -0.04;
  if(scale > 1.5) direction = 0;
  else if(scale < 0) {
    direction = 1;
    image_switch = (image_switch + 1) % 18;
  }

  roll = (roll + 2) % 360 ;
}

/**
 * @brief Samples a color from the source image using nearest-neighbor sampling.
 * @param src Pointer to the source image buffer
 * @param srcW Width of the source image
 * @param srcH Height of the source image
 * @param u The floating-point x-coordinate to sample
 * @param v The floating-point y-coordinate to sample
 * @return The 16-bit pixel value, or 0 (black) if out of bounds.
 */
static inline uint16_t sample_nearest(const uint16_t *src, int srcW, int srcH, float u, float v) {
    // Round to nearest integer coordinate
    int x = (int)(u + 0.5f);
    int y = (int)(v + 0.5f);

    // Check bounds
    if (x < 0 || x >= srcW || y < 0 || y >= srcH) {
        return 0; // Return background color (black)
    }

    return src[y * srcW + x];
}

/**
 * @brief Rotates a 2D image in 3D space using inverse mapping and nearest-neighbor
 * interpolation. This prevents holes but will look blocky/aliased.
 *
 * @param src Pointer to the source image buffer (16-bit)
 * @param dst Pointer to the destination image buffer (16-bit)
 * @param srcW Width of the source image
 * @param srcH Height of the source image
 * @param dstW Width of the destination image
 * @param dstH Height of the destination image
 * @param pitch Rotation around X-axis (in degrees)
 * @param yaw Rotation around Y-axis (in degrees)
 * @param roll Rotation around Z-axis (in degrees)
 * @param scale Scaling factor applied to the source image before rotation
 * @param fov Field of View of the virtual camera (in degrees)
 */
void rotateImage3D_Inverse(
    const uint16_t *src, uint16_t *dst,
    int srcW, int srcH,
    int dstW, int dstH,
    float pitch, float yaw, float roll,
    float scale,
    float fov)
{
    // --- 1. Setup transformations (same as original) ---
    float p = pitch * M_PI / 180.0f;
    float y = yaw   * M_PI / 180.0f;
    float r = roll  * M_PI / 180.0f;

    float cosP = cosf(p), sinP = sinf(p);
    float cosY = cosf(y), sinY = sinf(y);
    float cosR = cosf(r), sinR = sinf(r);

    // Rotation matrix (Rz * Rx * Ry)
    // This matrix transforms from model space (source image plane, Z=0) to world space
    float m00 = cosY * cosR + sinY * sinP * sinR;
    float m01 = cosP * sinR;
    // float m02 = -sinY * cosR + cosY * sinP * sinR; // Not needed for Z=0 source

    float m10 = -cosY * sinR + sinY * sinP * cosR;
    float m11 = cosP * cosR;
    // float m12 = sinR * sinY + cosY * sinP * cosR; // Not needed for Z=0 source

    float m20 = sinY * cosP;
    float m21 = -sinP;
    // float m22 = cosY * cosP; // Not needed for Z=0 source

    float cxS = srcW * 0.5f;
    float cyS = srcH * 0.5f;
    float cxD = dstW * 0.5f;
    float cyD = dstH * 0.5f;

    // Calculate focal length 'f' from FOV
    float fov_rad = fov * 0.5f * M_PI / 180.0f;
    if (fov_rad <= 0.0f) fov_rad = 0.001f; // Avoid division by zero
    float f = (dstW * 0.5f) / tanf(fov_rad);

    // --- 2. Clear destination buffer ---
    for (int i = 0; i < dstW * dstH; i++) {
        dst[i] = 0; // Clear to black
    }

    // --- 3. Inverse Mapping (Iterate over destination pixels) ---
    for (int yd = 0; yd < dstH; yd++) {
        for (int xd = 0; xd < dstW; xd++) {

            // (u_prime, v_prime) are normalized screen coords relative to center
            float u_prime = (float)xd - cxD;
            float v_prime = (float)yd - cyD;

            // We need to solve the 2x2 linear system to find (X, Y) in model space
            // from (u_prime, v_prime) in screen space.
            //
            // From the forward projection:
            // u' = (x3 * f) / z3_world = f * (m00*X + m01*Y) / (m20*X + m21*Y + f)
            // v' = (y3 * f) / z3_world = f * (m10*X + m11*Y) / (m20*X + m21*Y + f)
            //
            // Rearranging gives:
            // X * (u'*m20 - f*m00) + Y * (u'*m21 - f*m01) = -u'*f
            // X * (v'*m20 - f*m10) + Y * (v'*m21 - f*m11) = -v'*f
            //
            // Let this be:
            // a*X + b*Y = e
            // c*X + d*Y = g

            float a = u_prime * m20 - f * m00;
            float b = u_prime * m21 - f * m01;
            float c = v_prime * m20 - f * m10;
            float d = v_prime * m21 - f * m11;

            float e = -u_prime * f;
            float g = -v_prime * f;

            // Determinant of the 2x2 matrix
            float det = a * d - b * c;

            // If determinant is near zero, the pixel maps to infinity
            // (e.g., looking at the plane edge-on). Skip it.
            if (fabsf(det) < 1e-6f) { // Use 1e-6f for float
                continue;
            }

            float det_inv = 1.0f / det;

            // Solve for (X, Y) using Cramer's rule
            float X = (e * d - b * g) * det_inv;
            float Y = (a * g - e * c) * det_inv;

            // --- 4. Convert (X, Y) back to source pixel coordinates ---
            // (X, Y) are local coords relative to src center.
            // Apply inverse scale and offset.
            float x_src_float = (X * scale) + cxS;
            float y_src_float = (Y * scale) + cyS;

            // --- 5. Sample source image using nearest-neighbor ---
            // The sample_nearest function handles bounds checking.
            dst[yd * dstW + xd] = sample_nearest(src, srcW, srcH, x_src_float, y_src_float);
        }
    }
}