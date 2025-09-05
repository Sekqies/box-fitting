#ifndef MATHARRAY_H
#define MATHARRAY_H
#include <array>
#include <type_traits>

using namespace std;

template <typename T, size_t N> class MathArray;

template<typename> struct is_math_array : std::false_type {};

template<typename T, size_t N>
struct is_math_array<MathArray<T,N>> : std::true_type {};

template <typename T, size_t N>
class MathArray {
private:
    array<T, N> ar;

public:
    MathArray() = default;
    MathArray(std::initializer_list<T> init) {
        size_t i = 0;
        for (auto it = init.begin(); it != init.end() && i < N; ++it, ++i) {
            ar[i] = *it;
        }
        // Optionally, zero-fill remaining elements if init.size() < N
        for (; i < N; ++i) {
            ar[i] = T{};
        }
    }
    template<typename U, size_t M>
    U getsum(const MathArray<U,M>& u) const {
        U sum = U(0);
        for(size_t i = 0; i < M; i++){
            sum += u[i];
        }
        return sum;
    }

    void normalize() {
        normalize(*this);
    }

    template<typename U, size_t M>
    typename enable_if<!is_math_array<U>::value, void>::type
    normalize(MathArray<U, M>& u) {
        U sum = getsum(u);
        u = u * (1.0 / sum);
    }

    template<typename U, size_t M>
    typename enable_if<is_math_array<U>::value, void>::type
    normalize(MathArray<U, M>& u) {
        for (size_t i = 0; i < M; i++) {
            normalize(u[i]);
        }
    }
    auto begin(){
        return ar.begin();
    }
    auto end(){
        return ar.end();
    }

    T& operator[](size_t index) {
        return ar[index]; 
    }

    const T& operator[](size_t index) const {
        return ar[index];
    }

    MathArray operator+(const MathArray& v) const {
        MathArray out;
        for(size_t i = 0; i < N; i++){
            out[i] = ar[i] + v[i];
        }
        return out;
    }

    MathArray operator*(const double a) const {
        MathArray out;
        for(size_t i = 0; i < N; i++){
            out[i] = ar[i] * a;
        }
        return out;
    }
};
#endif