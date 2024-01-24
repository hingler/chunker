#ifndef FRACTION_H_
#define FRACTION_H_

#include <algorithm>
#include <numeric>

namespace chunker {
  namespace util {
    struct Fraction {
      int numerator;
      int denominator;

      Fraction() {
        numerator = 1;
        denominator = 1;
      }

      Fraction(int num, int den) {
        int gcd = std::gcd(num, den);
        numerator = num / gcd;
        denominator = den / gcd;
      }

      double AsDouble() const {
        return static_cast<double>(numerator) / denominator;
      }

      Fraction& operator=(int other) {
        numerator = other;
        denominator = 1;
        return *this;
      }

      bool operator==(const Fraction& other) const {
        return numerator * other.denominator == (other.numerator * denominator);
      }

      bool operator<(const Fraction& rhs) const {
        return (numerator * rhs.denominator) < (rhs.numerator * denominator);
      }

      bool operator>(const Fraction& rhs) const {
        return (numerator * rhs.denominator) > (rhs.numerator * denominator);
      }

      bool operator<=(const Fraction& rhs) const {
        return (*this < rhs) || (*this == rhs);
      }

      bool operator>=(const Fraction& rhs) const {
        return (*this > rhs) || (*this == rhs);
      }

      Fraction operator*(const Fraction& other) {
        return Fraction(numerator * other.numerator, denominator * other.denominator);
      }

      Fraction operator/(const Fraction& other) {
        return Fraction(numerator * other.denominator, denominator * other.numerator);
      }

      int operator*(int rhs) const {
        return (rhs * numerator) / denominator;
      }

      size_t operator*(size_t rhs) const {
        // can generalize for all int types, ideally
        return (rhs * numerator) / denominator;
      }

      Fraction operator/(int rhs) {
        return Fraction(numerator, denominator * rhs);
      }
    };   
  }
}

namespace std {
  template <>
  struct hash<chunker::util::Fraction> {
    std::hash<int> int_hash;
    size_t operator()(const chunker::util::Fraction& fraction) const {
      return int_hash(fraction.numerator) ^ int_hash(fraction.denominator);
    }
  };
}

#endif // FRACTION_H_