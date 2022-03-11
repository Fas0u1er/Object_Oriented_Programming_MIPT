#include <iostream>
#include <vector>
#include <string>

class BigInteger;
bool operator<(const BigInteger&, const BigInteger&);
bool operator>(const BigInteger&, const BigInteger&);
bool operator==(const BigInteger&, const BigInteger&);
bool operator!=(const BigInteger&, const BigInteger&);
bool operator>=(const BigInteger&, const BigInteger&);
bool operator<=(const BigInteger&, const BigInteger&);

class BigInteger {
private:

    bool if_negative = false; //if equals to "0" also false
    std::vector<long long> digits; //digits in reverse order, with no leading zeros,  digits[i] < BASE

    void cut_zeros() {
        while (digits_num() > 1 && digits.back() == 0) {
            digits.pop_back();
        }
        if (digits.back() == 0)
            if_negative = false;
    }


    BigInteger iteration_of_division(const BigInteger& divisor) {
        //find max (digit * BASE**t) * divisor that less than *this
        // and subtract it from *this and return (digit * BASE**t);
        int t = static_cast<int>(digits_num() - divisor.digits_num());
        BigInteger shifted_divisor = 0;
        long long main_digit = 0;
        if (divisor.get_mult_by_digit_unsigned(1, t) > *this) {
            --t;
            shifted_divisor = divisor.get_mult_by_digit_unsigned(1, t);
            main_digit = digits[digits_num() - 1] * BASE + digits[digits_num() - 2];

        } else {
            shifted_divisor = divisor.get_mult_by_digit_unsigned(1, t);
            main_digit = digits[digits_num() - 1];
        }
        long long l = (main_digit / (divisor.digits[divisor.digits_num() - 1] + 1));
        long long r = (main_digit / divisor.digits[divisor.digits_num() - 1]) + 1;
        while (l < r - 1) {
            long long mid = (l + r) >> 1;
            if (shifted_divisor.get_mult_by_digit_unsigned(mid) <= *this) {
                l = mid;
            } else {
                r = mid;
            }
        }
        *this -= shifted_divisor.get_mult_by_digit_unsigned(l);
        return BigInteger(static_cast<int>(l)).get_mult_by_digit_unsigned(1, t);
    }

    bool that_abs_less(const BigInteger& that) const {
        if (that.digits_num() < digits_num()) {
            return true;
        }
        if (that.digits_num() == digits_num()) {
            for (int i = static_cast<int>(digits_num()) - 1; i >= 0; --i) {
                if (digits[i] == that.digits[i]) {
                    continue;
                }
                return (digits[i] > that.digits[i]);
            }
        }
        return false;
    }

    void add_with_sign(const BigInteger& add, int sign) {
        //do not mind sign(this), sign(add), but abs(this) >= abs(add)
        long long carry = 0;
        for (size_t i = 0; (i < digits_num() && i < add.digits_num()) || carry; ++i) {
            if (i >= digits_num()) {
                digits.push_back(0);
            }
            if (add.digits_num() > i) {
                digits[i] += add.digits[i] * sign;
            }
            digits[i] += carry;
            carry = 0;

            if (digits[i] < 0) {
                digits[i] += BASE;
                carry = -1;
            }
            if (digits[i] >= BASE) {
                digits[i] -= BASE;
                carry = 1;
            }
        }

        cut_zeros();
    }

    friend std::istream& operator>>(std::istream&, BigInteger&);
    friend bool operator<(const BigInteger&, const BigInteger&);
    friend bool operator==(const BigInteger&, const BigInteger&);
    friend bool operator!=(const BigInteger&, const BigInteger&);

public:

    static const long long BASE = 10000;
    static const int DIGIT_LENGTH = 4;

    BigInteger get_mult_by_digit_unsigned(long long to_mult, size_t shift = 0) const {
        BigInteger result = 0;
        result.digits.clear();
        for (size_t i = 0; i < shift; ++i) {
            result.digits.push_back(0);
        }
        for (size_t i = shift; i < shift + digits_num(); ++i) {
            result.digits.push_back(digits[i - shift]);
        }
        if (to_mult == 1)
            return result;
        long long carry = 0;
        for (size_t i = 0; i < result.digits_num() || carry; ++i) {
            if (i >= result.digits_num()) {
                result.digits.push_back(0);
            }
            result.digits[i] = result.digits[i] * to_mult + carry;
            carry = result.digits[i] / BASE;
            result.digits[i] %= BASE;
        }

        result.cut_zeros();
        return result;
    }

    void swap_with(BigInteger& that) {
        std::swap(digits, that.digits);
        std::swap(if_negative, that.if_negative);
    }

    size_t digits_num() const {
        // digit_size = 10000;
        return digits.size();
    }

    BigInteger(const BigInteger&) = default;

    BigInteger(int x = 0) : if_negative(false) {
        if (x < 0) {
            x = -x;
            if_negative = true;
        }
        while (x) {
            digits.push_back(x % BASE);
            x /= BASE;
        }
        if (digits_num() == 0)
            digits.push_back(0);
    }

    BigInteger& operator=(const BigInteger& that) {
        BigInteger new_val(that);
        std::swap(digits, new_val.digits);
        std::swap(if_negative, new_val.if_negative);
        return *this;
    }

    ~BigInteger() = default;

    explicit operator bool() const {
        return (digits_num() > 1) || digits[0] != 0;
    }

    std::string toString() const {
        std::string str;
        str.reserve(digits_num() * DIGIT_LENGTH + 1);
        if (if_negative)
            str += '-';
        std::string digit_str = "";
        for (int i = static_cast<int>(digits_num()) - 1; i >= 0; --i) {
            digit_str = std::to_string(digits[i]);
            if (i == static_cast<int>(digits_num()) - 1) {
                str += digit_str;
                continue;
            }
            for (int j = 0; j + digit_str.size() < DIGIT_LENGTH; ++j) {
                str += '0';
            }
            str += digit_str;
        }

        return str;
    }

    BigInteger operator-() const {
        BigInteger result(*this);
        if (*this == 0)
            return result;

        result.if_negative ^= 1;
        return result;
    }

    BigInteger& operator+=(const BigInteger& that) {
        if (that_abs_less(that)) {
            int sign = (if_negative != that.if_negative) ? -1 : 1;
            add_with_sign(that, sign);
            return *this;
        }

        if (that.digits == digits) {
            if (if_negative == that.if_negative) {
                add_with_sign(that, 1);
                return *this;
            }
            *this = 0;
            return *this;
        }

        BigInteger result = that;
        int sign = (if_negative != that.if_negative) ? -1 : 1;
        result.add_with_sign(*this, sign);
        std::swap(digits, result.digits);
        std::swap(if_negative, result.if_negative);
        return *this;
    }

    BigInteger& operator-=(const BigInteger& that) {
        if (that_abs_less(that)) {
            int sign = (if_negative == that.if_negative) ? -1 : 1;
            add_with_sign(that, sign);
            return *this;
        }

        if (that.digits == digits) {
            if (if_negative != that.if_negative) {
                add_with_sign(that, 1);
                return *this;
            }
            *this = 0;
            return *this;
        }

        BigInteger result = that;
        int sign = (if_negative == that.if_negative) ? -1 : 1;
        result.add_with_sign(*this, sign);
        result.if_negative ^= 1;
        std::swap(digits, result.digits);
        std::swap(if_negative, result.if_negative);
        return *this;
    }

    BigInteger& operator*=(const BigInteger& that) {
        if (*this == 0 || that == 0) {
            *this = 0;
            return *this;
        }

        BigInteger result = 0;
        for (size_t i = 0; i < digits_num(); ++i) {
            result += (that.get_mult_by_digit_unsigned(digits[i], i));
        }
        result.if_negative = if_negative ^ that.if_negative;
        std::swap(if_negative, result.if_negative);
        std::swap(digits, result.digits);
        return *this;
    }

    BigInteger& operator/=(BigInteger that) {
        if (*this == 0)
            return *this;
        bool if_result_negative = if_negative ^ that.if_negative;
        that.if_negative = false;
        if_negative = false;
        BigInteger result = 0;
        while (*this >= that) {
            result += iteration_of_division(that);
        }

        std::swap(digits, result.digits);
        if (*this != 0)
            if_negative = if_result_negative;
        return *this;
    }

    BigInteger& operator++() {
        return (*this += 1);
    }

    BigInteger& operator--() {
        return (*this -= 1);
    }

    BigInteger operator++(int) {
        BigInteger copy = *this;
        ++(*this);
        return copy;
    }

    BigInteger operator--(int) {
        BigInteger copy = *this;
        --(*this);
        return copy;
    }
};

bool operator<(const BigInteger& left_arg, const BigInteger& right_arg) {
    if (left_arg.if_negative && !right_arg.if_negative) {
        return true;
    }
    if (!left_arg.if_negative && right_arg.if_negative) {
        return false;
    }

    if (right_arg.digits_num() > left_arg.digits_num()) {
        return !left_arg.if_negative;
    }
    if (right_arg.digits_num() == left_arg.digits_num()) {
        for (int i = static_cast<int>(left_arg.digits_num()) - 1; i >= 0; --i) {
            if (left_arg.digits[i] == right_arg.digits[i]) {
                continue;
            }
            return (left_arg.digits[i] < right_arg.digits[i]) ^ left_arg.if_negative;
        }
    }

    return false;
}

bool operator>(const BigInteger& left_arg, const BigInteger& right_arg) {
    return (right_arg < left_arg);
}

bool operator==(const BigInteger& left_arg, const BigInteger& right_arg) {
    return (left_arg.if_negative == right_arg.if_negative) && (left_arg.digits == right_arg.digits);
}

bool operator!=(const BigInteger& left_arg, const BigInteger& right_arg) {
    return (left_arg.if_negative != right_arg.if_negative) || (left_arg.digits != right_arg.digits);
}

bool operator<=(const BigInteger& left_arg, const BigInteger& right_arg) {
    return (left_arg < right_arg || left_arg == right_arg);
}

bool operator>=(const BigInteger& left_arg, const BigInteger& right_arg) {
    return (right_arg < left_arg || left_arg == right_arg);
}

BigInteger operator+(const BigInteger& x, const BigInteger& y) {
    BigInteger result = x;
    return (result += y);
}

BigInteger operator-(const BigInteger& x, const BigInteger& y) {
    BigInteger result = x;
    return (result -= y);
}

BigInteger operator*(const BigInteger& x, const BigInteger& y) {
    BigInteger result = x;
    return (result *= y);
}

BigInteger operator/(const BigInteger& x, const BigInteger& y) {
    BigInteger result = x;
    return (result /= y);
}

BigInteger& operator%=(BigInteger& x, const BigInteger& y) {
    if (x >= 0 && y >= 0)
        return x -= ((x / y) * y);
    if (x < 0 && y >= 0)
        return x += (((-x) / y) * y);
    if (x >= 0 && y < 0)
        return x -= ((x / (-y)) * (-y));
    if (x < 0 && y < 0)
        return x += (((-x) / (-y)) * (-y));
    return x;
}

BigInteger operator%(const BigInteger& x, const BigInteger& y) {
    BigInteger result = x;
    return (result %= y);
}


std::ostream& operator<<(std::ostream& out, const BigInteger& to_out) {
    out << to_out.toString();
    return out;
}

std::istream& operator>>(std::istream& in, BigInteger& to_get) {
    to_get.digits.clear();
    to_get.if_negative = false;

    std::string str;
    in >> str;

    int number_begin = 0;
    if (str[0] == '-') {
        to_get.if_negative = true;
        number_begin = 1;
    }

    for (int i = str.length() - 1; i >= number_begin; --i) {
        long long digit_val = 0;
        for (int j = std::max(i - BigInteger::DIGIT_LENGTH + 1, number_begin); j <= i; ++j) {
            digit_val = 10 * digit_val + str[j] - '0';
        }
        i = std::max(i - BigInteger::DIGIT_LENGTH + 1, number_begin);
        to_get.digits.push_back(digit_val);
    }
    return in;
}

BigInteger gcd(BigInteger x, BigInteger y) {
    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    while (y) {
        x %= y;
        x.swap_with(y);
    }
    return x;
}

class Rational {
private:
    BigInteger numerator;
    BigInteger denominator;

    void normalize() {
        BigInteger tmp = gcd(numerator, denominator);
        numerator /= tmp;
        denominator /= tmp;
    }

    friend bool operator<(const Rational&, const Rational&);
    friend bool operator==(const Rational&, const Rational&);
public:
    template<class T>
    explicit Rational(const T& x): numerator(x), denominator(1) {}

    explicit Rational(const BigInteger& x, const BigInteger& y) : numerator(x), denominator(y) {
        normalize();
    }

    Rational(const BigInteger& x) : numerator(x), denominator(1) {}

    Rational(int x = 0) : numerator(x), denominator(1) {}

    Rational& operator=(const Rational&) = default;

    std::string toString() {
        if (denominator == 1) {
            return numerator.toString();
        }

        return numerator.toString() + '/' + denominator.toString();
    }

    std::string asDecimal(size_t precision = 0) const {
        if (precision == 0)
            return (numerator / denominator).toString();
        BigInteger shifted_result = numerator;
        //shifting numerator by 10**precision:
        shifted_result = shifted_result.get_mult_by_digit_unsigned(1, precision / BigInteger::DIGIT_LENGTH);
        int to_mult = 1;
        for (size_t i = 0; i < (precision % BigInteger::DIGIT_LENGTH); ++i) {
            to_mult *= 10;
        }
        shifted_result = shifted_result.get_mult_by_digit_unsigned(to_mult);
        shifted_result /= denominator;

        std::string resulting_digits = shifted_result.toString();
        std::string result;
        result.reserve(std::max(resulting_digits.size(), precision) + 2);
        if (numerator < 0) {
            result = '-';
        }
        if (resulting_digits.size() > precision) {
            for (size_t i = 0; i < resulting_digits.size() - precision; ++i) {
                result += resulting_digits[i];
            }
            result += ".";
            for (size_t i = resulting_digits.size() - precision; i < resulting_digits.size(); ++i) {
                result += resulting_digits[i];
            }
        } else {
            result += "0.";
            result += std::string(precision - resulting_digits.size(), '0');
            result += resulting_digits;
        }
        return result;
    }

    explicit operator double() const {
        return std::stod(asDecimal(18));
    }


    Rational& operator+=(const Rational& that) {
        numerator = (numerator * that.denominator) + (that.numerator * denominator);
        denominator *= that.denominator;
        normalize();
        return *this;
    }

    Rational& operator-=(const Rational& that) {
        numerator = (numerator * that.denominator) - (that.numerator * denominator);
        denominator *= that.denominator;
        normalize();
        return *this;
    }

    Rational& operator*=(const Rational& that) {
        numerator *= that.numerator;
        denominator *= that.denominator;
        normalize();
        return *this;
    }

    Rational& operator/=(const Rational& that) {
        numerator *= that.denominator;
        denominator *= that.numerator;
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
        normalize();
        return *this;
    }

    Rational operator-() const {
        Rational result(*this);
        result.numerator = -result.numerator;
        return result;
    }

};

bool operator<(const Rational& left_arg, const Rational& right_arg) {
    return (left_arg.numerator * right_arg.denominator) < (right_arg.numerator * left_arg.denominator);
}

bool operator>(const Rational& left_arg, const Rational& right_arg) {
    return right_arg < left_arg;
}

bool operator==(const Rational& left_arg, const Rational& right_arg) {
    return left_arg.numerator == right_arg.numerator && left_arg.denominator == right_arg.denominator;
}

bool operator!=(const Rational& left_arg, const Rational& right_arg) {
    return !(left_arg == right_arg);
}

bool operator<=(const Rational& left_arg, const Rational& right_arg) {
    return !(left_arg > right_arg);
}

bool operator>=(const Rational& left_arg, const Rational& right_arg) {
    return !(left_arg < right_arg);
}

Rational operator+(const Rational& x, const Rational& y) {
    Rational result(x);
    return (result += y);
}

Rational operator-(const Rational& x, const Rational& y) {
    Rational result(x);
    return (result -= y);
}

Rational operator*(const Rational& x, const Rational& y) {
    Rational result(x);
    return (result *= y);
}

Rational operator/(const Rational& x, const Rational& y) {
    Rational result(x);
    return (result /= y);
}
