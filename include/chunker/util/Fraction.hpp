#ifndef FRACTION_H_
#define FRACTION_H_

#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>
#include <type_traits>


namespace chunker {
  namespace util {
    struct Fraction {
      long numerator;
      long denominator;

      Fraction() {
        numerator = 1;
        denominator = 1;
      }

      Fraction(long num) {
        numerator = num;
        denominator = 1;
      }

      Fraction(long num, long den) {
        long gcd = std::gcd(num, den);
        numerator = num / gcd;
        denominator = den / gcd;
      }

      float AsFloat() const {
        return static_cast<float>(numerator) / denominator;
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
        return (numerator * other.denominator) == (other.numerator * denominator);
      }

      bool operator!=(const Fraction& other) const {
        return !(*this == other);
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

      Fraction operator*(const Fraction& other) const {
        return Fraction(numerator * other.numerator, denominator * other.denominator);
      }

      Fraction operator/(const Fraction& other) const {
        return Fraction(numerator * other.denominator, denominator * other.numerator);
      }

      Fraction operator+(const Fraction& other) const {
        // possibly-costly to be creating all these fractions
        return Fraction(numerator * other.denominator + other.numerator * denominator, denominator * other.denominator);
      }

      Fraction operator-(const Fraction& other) const {
        return Fraction(numerator * other.denominator - other.numerator * denominator, denominator * other.denominator);
      }

      template <typename Integral, std::enable_if_t<std::is_integral_v<Integral>, bool> = true>
      Fraction operator*(Integral rhs) const {
        return Fraction(rhs * numerator, denominator);
      }

      template <typename Integral, std::enable_if_t<std::is_integral_v<Integral>, bool> = true>
      Fraction operator/(Integral rhs) const {
        return Fraction(numerator, denominator * rhs);
      }

      // freaking out on vec
      template <typename ArithmeticType, std::enable_if_t<std::is_arithmetic_v<ArithmeticType>, bool> = true>
      operator ArithmeticType() const { return static_cast<ArithmeticType>(numerator) / static_cast<ArithmeticType>(denominator); }

      operator std::string() const {
        std::stringstream stream;
        stream << numerator << " / " << denominator;
        return stream.str();
      }

      template <typename IntegralType, std::enable_if_t<std::is_integral_v<IntegralType>, bool> = true>
      Fraction operator+(IntegralType rhs) const {
        return Fraction(numerator + rhs * denominator, denominator);
      }

      template <typename IntegralType, std::enable_if_t<std::is_integral_v<IntegralType>, bool> = true>
      Fraction operator-(IntegralType rhs) const {
        return Fraction(numerator - rhs * denominator, denominator);
      }
    };

    template <typename IntegralType, std::enable_if_t<std::is_integral_v<IntegralType>, bool> = true>
      Fraction operator+(IntegralType lhs, const Fraction& rhs) {
        return Fraction(rhs.numerator + lhs * rhs.denominator, rhs.denominator);
      }

      template <typename IntegralType, std::enable_if_t<std::is_integral_v<IntegralType>, bool> = true>
      Fraction operator-(IntegralType lhs, const Fraction& rhs) {
        return Fraction(rhs.numerator - lhs * rhs.denominator, rhs.denominator);
      }
  }
}

namespace std {
  template <>
  struct hash<chunker::util::Fraction> {
    std::hash<long> int_hash;
    size_t operator()(const chunker::util::Fraction& fraction) const {
      return int_hash(fraction.numerator) ^ int_hash(fraction.denominator);
    }
  };
}

#endif // FRACTION_H_