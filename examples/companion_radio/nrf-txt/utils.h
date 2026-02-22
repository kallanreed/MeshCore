#pragma once

class Utils {
public:
  static float toF(float c) {
    return (c * 9.0f / 5.0f) + 32.0f;
  }

  static float toInHg(float hpa) {
    return hpa * 0.0295299831f;
  }
};
