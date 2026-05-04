/*!
 * @author hongjun.liao <docici@126.com>
 *
 * my bitcoin core research
 * y^2 = x ^ 3 + 7
 * */

/////////////////////////////////////////////////////////////////////////////////////////////
#include "btc_inc.h"
#define ec_none(a, b) ec_point{(a), (b), 0, 0}

template <typename T>
T btc_ec_add(T a, T b, T x1, T y1, T x2, T y2, T & x, T & y)
{
	//至少有一个无穷远点
	if(x1 == y1 && y1 == 0) { x = x2; y = y2; return x2; };
	if(x2 == y2 && y2 == 0) { x = x1; y = y1; return x1; };
	if(x1 == x2 && x2 == 0 && y1 == y2 && y2 == 0) { x = y = 0; return 0; }
	if(x1 == x2 && y1 != y2) { x = y = 0; return 0; }
	//相同点， 相切
	if(x1 == x2 && y1 == y2){
		auto s = (3 * x1 * x1 + a) / (2 * y1);
		x = s * s - 2 * x1;
		y = s * (x1 - x) - y1;
		return x;
	}
	//2个不同点，计算斜率
	auto s = (y2 - y1) / (x2 - x1);
	x = s * s - x1 - x2;
	y = s * ( x1 - x) - y1;
	return x;
}

class btc_fe {
private:
    int num;
    int prime;

public:
    btc_fe(int num = 0):num(num), prime(num){}
    btc_fe(int num, int prime) : num(num), prime(prime) {
       brequire(!(num >= prime || num < 0));
    }

    std::string repr() const {
        return "btc_fe_" + std::to_string(prime) + "(" + std::to_string(num) + ")";
    }

    bool operator==(const btc_fe& other) const {
        return num == other.num && prime == other.prime;
    }

    bool operator!=(const btc_fe& other) const {
        return !(*this == other);
    }

    btc_fe operator+(const btc_fe& other) const {
    	brequire (prime == other.prime);
        int result_num = (num + other.num) % prime;
        return btc_fe(result_num, prime);
    }

    btc_fe operator-(const btc_fe& other) const {
    	brequire (prime == other.prime);
        int result_num = (num - other.num) % prime;
        // Ensure non-negative result
        if (result_num < 0) result_num += prime;
        return btc_fe(result_num, prime);
    }

    btc_fe operator*(const btc_fe& other) const {
    	brequire (prime == other.prime);
        int result_num = (num * other.num) % prime;
        return btc_fe(result_num, prime);
    }

    btc_fe pow(int exponent) const {
        // Using Fermat's little theorem: n = exponent % (prime - 1)
        int n = exponent % (prime - 1);
        int result_num = 1;
        int base = num;
        int exp = n;

        // Fast exponentiation modulo prime
        while (exp > 0) {
            if (exp & 1) {
                result_num = (result_num * base) % prime;
            }
            base = (base * base) % prime;
            exp >>= 1;
        }

        return btc_fe(result_num, prime);
    }

    btc_fe operator/(const btc_fe& other) const {
    	brequire (prime == other.prime);

        // Using Fermat's little theorem: 1/n == n^(p-2) mod p
        // Compute other.num^(prime-2) mod prime
        int inverse = 1;
        int base = other.num;
        int exp = prime - 2;

        while (exp > 0) {
            if (exp & 1) {
                inverse = (inverse * base) % prime;
            }
            base = (base * base) % prime;
            exp >>= 1;
        }

        int result_num = (num * inverse) % prime;
        return btc_fe(result_num, prime);
    }

    // Scalar multiplication (coefficient * field element)
    friend btc_fe operator*(int coefficient, const btc_fe& fe) {
        int result_num = (fe.num * coefficient) % fe.prime;
        return btc_fe(result_num, fe.prime);
    }

    // Getters (optional, for debugging)
    int getNum() const { return num; }
    int getPrime() const { return prime; }
};

template<typename T>
class ec_point{
	T _a, _b, _x, _y;
public:
	ec_point(T a, T b, T x, T y): _x(x),  _y(y), _a(a),  _b(b){
		if(!(x == y && y == 0)){
			auto l = y * y, r = x * x * x + a * x  + b;
			brequire(l == r);
		}
	}
	bool operator == (ec_point const & rhs) const {
		return _x == rhs._x && _y == rhs._y && _a == rhs._a && _b == rhs._b;
	}
	bool operator != (ec_point const & rhs) const { return !(*this == rhs); }
	ec_point operator +(ec_point const & p) const
	{
		if(!(_a == p._a && _b == p._b)) return ec_none(_a, _b);
		T x, y;
		btc_ec_add(_a, _b, _x, _y, p._x, p._y, x, y);
		return ec_point(_a, _b, x, y);
	}
	ec_point& operator +=(ec_point const & p)
	{
		return *this = *this + p;
	}
	ec_point operator *(int n) const
	{
		ec_point ret(*this);
		for(;n > 1;--n)
			ret = ret + *this;
		return ret;
	}
	int get_order() const
	{
		int n = 1;
		auto next = *this;
		while(next != ec_none(this->_a, this->_b)){
			next += *this;
			++n;
		}
		return n;
	}
};

BOOST_AUTO_TEST_SUITE(ecdsa)
BOOST_AUTO_TEST_CASE(fe) {
    // test_ne
    {
        btc_fe a(2, 31);
        btc_fe b(2, 31);
        btc_fe c(15, 31);

        brequire(a == b);
        brequire(a != c);
        brequire(!(a != b));
    }

    // test_add
    {
        btc_fe a(2, 31);
        btc_fe b(15, 31);
        brequire((a + b) == btc_fe(17, 31));

        btc_fe a2(17, 31);
        btc_fe b2(21, 31);
        brequire((a2 + b2) == btc_fe(7, 31));
    }

    // test_sub
    {
        btc_fe a(29, 31);
        btc_fe b(4, 31);
        brequire((a - b) == btc_fe(25, 31));

        btc_fe a2(15, 31);
        btc_fe b2(30, 31);
        brequire((a2 - b2) == btc_fe(16, 31));
    }

    // test_mul
    {
        btc_fe a(24, 31);
        btc_fe b(19, 31);
        brequire((a * b) == btc_fe(22, 31));
    }

    // test_rmul
    {
        btc_fe a(24, 31);
        int b = 2;
        brequire((b * a) == (a + a));
    }

    // test_pow
    {
        btc_fe a(17, 31);
        brequire(a.pow(3) == btc_fe(15, 31));

        btc_fe a2(5, 31);
        btc_fe b2(18, 31);
        brequire((a2.pow(5) * b2) == btc_fe(16, 31));
    }

    // test_div
    {
        btc_fe a(3, 31);
        btc_fe b(24, 31);
        brequire((a / b) == btc_fe(4, 31));

        btc_fe a2(17, 31);
        // a**-3 is equivalent to a^(p-4) where p=31, so p-4=27
        // Since exponent is negative, we compute a^(p-1-3) = a^(27)
        brequire(a2.pow(27) == btc_fe(29, 31));

        btc_fe a3(4, 31);
        btc_fe b3(11, 31);
        // a**-4 * b = a^(p-5) * b where p=31, so p-5=26
        brequire((a3.pow(26) * b3) == btc_fe(13, 31));
    }
}
BOOST_AUTO_TEST_CASE(ec_int)
{
	ec_point<int> p1{5, 7, -1, -1};

	int x, y; btc_ec_add(5, 7, 2, 5, -1, -1, x, y);
	brequire( x == 3 && y == -7);

	//不同点, 斜率
	{
		ec_point<int> p3 = ec_point<int>{5, 7, 2, 5} + ec_point<int>{5, 7, -1, -1}, P3{5, 7, 3, -7};
		brequire(p3 == P3);
	}
	//相同点,切线
	{
		ec_point<int> p3 = ec_point<int>{5, 7, -1, -1} + ec_point<int>{5, 7, -1, -1}, P3{5, 7, 18, 77};
		brequire(p3 == P3);
	}

}
BOOST_AUTO_TEST_CASE(ec_fe)
{
	ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(192, 223), btc_fe(105, 223)};
	// fe addition on ec
	{
		ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(170, 223), btc_fe(142, 223)};
		ec_point<btc_fe> p2{btc_fe(0, 223), btc_fe(7, 223), btc_fe(60, 223), btc_fe(139, 223)};
		ec_point<btc_fe> P3{btc_fe(0, 223), btc_fe(7, 223), btc_fe(220, 223), btc_fe(181, 223)};
		auto p3 = p1 + p2;
		brequire(p3 == P3);
	}
	{
		ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(47, 223), btc_fe(71, 223)};
		ec_point<btc_fe> p2{btc_fe(0, 223), btc_fe(7, 223), btc_fe(17, 223), btc_fe(56, 223)};
		ec_point<btc_fe> P3{btc_fe(0, 223), btc_fe(7, 223), btc_fe(215, 223), btc_fe(68, 223)};
		auto p3 = p1 + p2;
		brequire(p3 == P3);
	}
	{
		ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(143, 223), btc_fe(98, 223)};
		ec_point<btc_fe> p2{btc_fe(0, 223), btc_fe(7, 223), btc_fe(76, 223), btc_fe(66, 223)};
		ec_point<btc_fe> P3{btc_fe(0, 223), btc_fe(7, 223), btc_fe(47, 223), btc_fe(71, 223)};
		auto p3 = p1 + p2;
		brequire(p3 == P3);
	}
	// fe mul on ec
	{
		ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(192, 223), btc_fe(105, 223)};
		ec_point<btc_fe> P3{btc_fe(0, 223), btc_fe(7, 223), btc_fe(49, 223), btc_fe(71, 223)};
		auto p3 = p1 + p1;
		brequire(p3 == P3);
	}
	{
		ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(47, 223), btc_fe(71, 223)};
		ec_point<btc_fe> P3{btc_fe(0, 223), btc_fe(7, 223), btc_fe(194, 223), btc_fe(51, 223)};
		auto p3 = p1 + p1 + p1 + p1;
		brequire(p3 == P3);
	}
	{
		ec_point<btc_fe> p1{btc_fe(0, 223), btc_fe(7, 223), btc_fe(47, 223), btc_fe(71, 223)};
		ec_point<btc_fe> P3{btc_fe(0, 223), btc_fe(7, 223), btc_fe(194, 223), btc_fe(51, 223)};
		auto p3 = p1 * 4;
		brequire(p3 == P3);
	}
	//calc order
	{
		ec_point<btc_fe> p{btc_fe(0, 223), btc_fe(7, 223), btc_fe(15, 223), btc_fe(86, 223)};
		brequire(p.get_order() == 7);
	}

}
BOOST_AUTO_TEST_SUITE_END()
