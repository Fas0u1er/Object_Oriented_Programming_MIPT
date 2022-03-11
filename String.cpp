#include <cstring>
#include <iostream>

class String {
public:
    String(const char* c_string) : capacity(0), len(strlen(c_string)) {
        capacity = len > 16 ? len : 16;
        buffer = new char[capacity];
        memcpy(buffer, c_string, len * sizeof(char));
    }

    String(size_t size, char c) : buffer(new char[size]), capacity(size), len(size) {
        memset(buffer, c, len * sizeof(char));
    }

    explicit String(size_t size = 16) : buffer(new char[size]), capacity(size), len(0) {}

    String(const String& src) : buffer(new char[src.capacity]), capacity(src.capacity), len(src.len) {
        memcpy(buffer, src.buffer, capacity * sizeof(char));
    }

    String& operator=(const String& str) &{
        String copy(str);
        swap_this_with(copy);
        return *this;
    }

    char& operator[](size_t index) {
        return buffer[index];
    }

    char operator[](size_t index) const {
        return buffer[index];
    }

    size_t length() const {
        return len;
    }

    void push_back(char c) {
        if (capacity == len)
            double_buffer();
        buffer[len++] = c;
    }

    void pop_back() {// Стандарт это святое
        buffer[--len]; 
    }

    char& front() {
        return buffer[0];
    }
    char front() const{
        return buffer[0];
    }

    char back() const{
        return buffer[len - 1];
    }
    char& back(){
        return buffer[len - 1];
    }

    String& operator+=(char c) {
        push_back(c);
        return *this;
    }

    String& operator+=(const String& that) { 
        change_buffer(static_cast<int>(that.len + len) / static_cast<int>(capacity) + 1);
        memcpy(buffer + len, that.buffer, that.len * sizeof(char));
        len += that.len;
        return *this;
    }

    String operator+(char c) {
        String sum(*this);
        sum.push_back(c);
        return sum;
    }

    friend String operator+(char c, const String& str);
	

    size_t find(const String& substring) const {
        if (len < substring.len)
            return length();
        for (size_t i = 0; i <= len - substring.len; ++i) {
            bool if_equal = true;
            for (size_t j = 0; j < substring.len; ++j) {
                if (buffer[i + j] != substring[j]) {
                    if_equal = false;
                    break;
                }
            }
            if (if_equal)
                return i;
        }
        return length();
    }

    size_t rfind(const String& substring) const {
        if (len < substring.len)
            return length();
        for (int i = static_cast<int>(len - substring.len); i >= 0; --i) {
            bool if_equal = true;
            for (size_t j = 0; j < substring.len; ++j) {
                if (buffer[i + j] != substring[j]) {
                    if_equal = false;
                    break;
                }
            }
            if (if_equal)
                return i;
        }
        return length();
    }

    String substr(size_t start, size_t count) const& {
        String substring(count, '0');
        memcpy(substring.buffer, buffer + start, count * sizeof(char));
        return substring;
    }

    bool empty() const {
        return len == 0;
    }

    void clear() {
        len = 0;
        delete[] buffer;
        buffer = new char[16];
        capacity = 16;
    }

    friend std::ostream& operator<<(std::ostream& output, const String& str);

    friend std::istream& operator>>(std::istream& input, String& str);

    ~String() {
        delete[] buffer;
    }

private:
    char* buffer;
    size_t capacity;
    size_t len;

    void change_buffer(int multiplier) {
        if (multiplier == 1)
            return;
        char* new_buffer = new char[multiplier * capacity];
        memcpy(new_buffer, buffer, capacity * sizeof(char));
        delete[] buffer;
        buffer = new_buffer;
        capacity *= multiplier;
    }

    void double_buffer() {
        change_buffer(2);
    }

    void swap_this_with(String& str) {
        std::swap(str.capacity, capacity);
        std::swap(str.len, len);
        std::swap(str.buffer, buffer);
    }
};

    bool operator==(const String& str1, const String& str2) {
        if(str1.length() != str2.length())
            return false;
        for (int i = 0; i < static_cast<int>(str1.length()); ++i) {
            if(str1[i] != str2[i])
                return false;
        }
        return true;
    }


	String operator+(char c, const String& str) {
    String sum(1, c);
    sum += str;
    return sum;
}

    String operator+(const String& add1, const String& add2) {
        String sum(add1);
        sum += add2;
        return sum;
    }

std::ostream& operator<<(std::ostream& output, const String& str) {
    for (size_t i = 0; i < str.len; ++i)
        output << str.buffer[i];
    return output;
}

std::istream& operator>>(std::istream& input, String& str) {
    str.clear();
    char c = ' ';
    while (c == ' ' || c == '\n') {
        if(!input.get(c)){
            return input;
        }
    }

    do {
        str.push_back(c);
        if(!input.get(c)){
            return input;
        }
    } while (c != ' ' && c != '\n');
    return input;
}
