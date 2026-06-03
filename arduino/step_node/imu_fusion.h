/*
 * Madgwick AHRS for ICM-20948 raw int16 (±2 g accel, ±250 dps gyro defaults).
 * Produces unit quaternion and gravity-removed linear acceleration for filter mode.
 */
#pragma once

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct ImuFusion {
  float q0 = 1.0f;
  float q1 = 0.0f;
  float q2 = 0.0f;
  float q3 = 0.0f;
  float beta = 0.1f;
  bool initialized = false;
};

static inline float invSqrt(float x) {
  return 1.0f / sqrtf(x);
}

// Accel LSB -> g; gyro LSB -> rad/s (ICM20948 defaults: 16384 LSB/g, 131 LSB/(deg/s) @ ±250 dps)
static inline void fusionUpdate(ImuFusion *f, const int16_t raw[6], float dt_s) {
  const float accel_scale = 1.0f / 16384.0f;
  const float gyro_scale = (250.0f / 131.0f) * (M_PI / 180.0f);

  float ax = raw[0] * accel_scale;
  float ay = raw[1] * accel_scale;
  float az = raw[2] * accel_scale;
  float gx = raw[3] * gyro_scale;
  float gy = raw[4] * gyro_scale;
  float gz = raw[5] * gyro_scale;

  if (!f->initialized) {
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 1e-3f) return;
    ax /= norm;
    ay /= norm;
    az /= norm;
    float roll = atan2f(ay, az);
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    f->q0 = cr * cp;
    f->q1 = sr * cp;
    f->q2 = cr * sp;
    f->q3 = -sr * sp;
    f->initialized = true;
    return;
  }

  if (dt_s < 1e-5f) return;

  float q0 = f->q0;
  float q1 = f->q1;
  float q2 = f->q2;
  float q3 = f->q3;

  float recipNorm = invSqrt(ax * ax + ay * ay + az * az);
  ax *= recipNorm;
  ay *= recipNorm;
  az *= recipNorm;

  float _2q0 = 2.0f * q0;
  float _2q1 = 2.0f * q1;
  float _2q2 = 2.0f * q2;
  float _2q3 = 2.0f * q3;
  float _4q0 = 4.0f * q0;
  float _4q1 = 4.0f * q1;
  float _4q2 = 4.0f * q2;
  float _8q1 = 8.0f * q1;
  float _8q2 = 8.0f * q2;
  float q0q0 = q0 * q0;
  float q0q1 = q0 * q1;
  float q0q2 = q0 * q2;
  float q1q1 = q1 * q1;
  float q1q3 = q1 * q3;
  float q2q2 = q2 * q2;
  float q2q3 = q2 * q3;
  float q3q3 = q3 * q3;

  float s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
  float s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
  float s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
  float s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
  recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
  s0 *= recipNorm;
  s1 *= recipNorm;
  s2 *= recipNorm;
  s3 *= recipNorm;

  float qDot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - f->beta * s0;
  float qDot1 = 0.5f * (q0 * gx + q2 * gz - q3 * gy) - f->beta * s1;
  float qDot2 = 0.5f * (q0 * gy - q1 * gz + q3 * gx) - f->beta * s2;
  float qDot3 = 0.5f * (q0 * gz + q1 * gy - q2 * gx) - f->beta * s3;

  q0 += qDot0 * dt_s;
  q1 += qDot1 * dt_s;
  q2 += qDot2 * dt_s;
  q3 += qDot3 * dt_s;

  recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
  f->q0 = q0 * recipNorm;
  f->q1 = q1 * recipNorm;
  f->q2 = q2 * recipNorm;
  f->q3 = q3 * recipNorm;
}

static inline void fusionLinearAccel(const ImuFusion *f, const int16_t raw[6], float out_g[3]) {
  const float accel_scale = 1.0f / 16384.0f;
  float ax = raw[0] * accel_scale;
  float ay = raw[1] * accel_scale;
  float az = raw[2] * accel_scale;

  float q0 = f->q0;
  float q1 = f->q1;
  float q2 = f->q2;
  float q3 = f->q3;

  float gx = 2.0f * (q1 * q3 - q0 * q2);
  float gy = 2.0f * (q0 * q1 + q2 * q3);
  float gz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

  out_g[0] = ax - gx;
  out_g[1] = ay - gy;
  out_g[2] = az - gz;
}

static inline int16_t quatToInt16(float q) {
  if (q > 1.0f) q = 1.0f;
  if (q < -1.0f) q = -1.0f;
  return (int16_t)(q * 32767.0f);
}

static inline int16_t accelGToInt16(float g) {
  if (g > 2.0f) g = 2.0f;
  if (g < -2.0f) g = -2.0f;
  return (int16_t)(g * 16384.0f);
}
