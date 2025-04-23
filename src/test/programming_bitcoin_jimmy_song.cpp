// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cmath>
#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

struct MyFixture {
    MyFixture() : data(42) {
        BOOST_TEST_MESSAGE("Setting up fixture");
    }
    ~MyFixture() {
        BOOST_TEST_MESSAGE("Tearing down fixture");
    }
    int data; // 共享数据
};

BOOST_FIXTURE_TEST_SUITE(programming_bitcoin_jimmy_song,MyFixture)

/*!
 * 有限域加法
 */
static int pbjs_fe_add(int prime, int e1, int e2)
{
	return (e1 + e2) % prime;
}

/*!
 * ec加法
 * @param p: 阶数
 * @param a,b,c:ec系数,y^2=ax^3 + bx + c
 * @param x1,y1:点p1
 * @param x2,y2:点p2
 * @param x3,y3加法结果,点p3
 * @return: 0 for OK
 *
 */
static int pbjs_ecfe_add(int p, int a, int b, int c, int x1, int y1, int x2, int y2, int& x3, int& y3)
{
	if(p < 1) { //非有限域上的ec加法
		//点在ec上
		int left, right;
		left = (int)(a * std::pow(x1, 3) + b * x1 + c), right = (int)std::pow(y1, 2);
		if(left != right) return -2;
		left = (int)(a * std::pow(x2, 3) + b * x2 + c), right = (int)std::pow(y2, 2);
		if(left != right) return -3;

		if(x1 == x2 && y1 != y2) { x3 = y3 = 0; } //直线垂直
		else if(x1 == x2 && y1 == y2 && y1 == 0) { x3 = y3 = 0; } //直线垂直,且y=0
		//p1,p2中有"无穷"远点
		else if(x1 == y1 && y1 == 0) { x3 = x2; y3 = y2; }
		else if(x2 == y2 && y2 == 0) { x3 = x1; y3 = y1; }
		else if(x1 == x2 && y1 == y2) { //椭圆曲线的切线
			auto s = (3 * std::pow(x1, 2) + b) / (2 * y1);
			x3 = (int)std::pow(s, 2) - 2 * x1;
			y3 = s * (x1 - x3) - y1;
		}
		else {	//普通斜线与ce相交,用斜率公式
			auto s = (y2 - y1) / (double)(x2 - x1);
			x3 = (int)std::pow(s, 2) - x1 - x2;
			y3 = s * (x1 - x3) - y1;
		}
	}else{ //有限域上的ec加法
		//点在ec上
		int left, right;
		left = (int)(a * std::pow(x1, 3) + b * x1 + c) % p, right = (int)std::pow(y1, 2) % p;
		if(left != right) return -2;
		left = (int)(a * std::pow(x2, 3) + b * x2 + c) % p, right = (int)std::pow(y2, 2) % p;
		if(left != right) return -3;

		if(x1 == x2) { x3 = y3 = 0; } //直线垂直
		//p1,p2中有"无穷"远点
		else if(x1 == y1 && y1 == 0) { x3 = x2; y3 = y2; }
		else if(x2 == y2 && y2 == 0) { x3 = x1; y3 = y1; }
		else {	//普通斜线与ce相交,用斜率公式
			auto s = (y2 - y1) / (double)(x2 - x1);
			x3 = (int)std::pow(s, 2) - x1 - x2;
			y3 = s * (x1 - x3) - y1;
		}
	}
	return 0;
}

BOOST_AUTO_TEST_CASE(test_pbjs_fe)
{
	int rc;
	int e = pbjs_fe_add(19, 2, 3); assert(e == 5);
	{
		//判断下面的点是否在有限域F223上的椭圆曲线y2 =x3 +7上。 (192,105),(17,56),(200,119),(1,193),(42,99)
		int left = (int)(std::pow(192,3) + 7) % 223, right = (int)std::pow(105,2) % 223;
		printf("(192,105): %d=%d,'%s'\n", left, right, (left == right? "Yes" : "NO"));
	}
	{
		//在椭圆曲线y2 =x3+5x+7上,计算(2,5)+(-1,-1)的结果。
		int x, y;
		rc = pbjs_ecfe_add(0, 1, 5, 7,2,5, -1,-1, x, y);
		assert(rc == 0 && x == 3 && y == -7);
	}
	{
		//在椭圆曲线y2 =x3+5x+7上,计算(-1,-1)+(-1,-1)。
		int x, y;
		rc = pbjs_ecfe_add(0, 1, 5, 7, -1, 1, -1, 1, x, y);
		assert(rc == 0 && x == 18 && y == -77);
	}
	{
		//对于有限域F223的椭圆曲线y2=x3+7计算: (170,142)+(60,139)
		int x, y;
		rc = pbjs_ecfe_add(223, 1, 0, 7,170,142, 60,139, x, y);
		assert(rc == 0 && x == 0 && y == 0);
	}
	assert(rc == 0);
}

BOOST_AUTO_TEST_SUITE_END()
