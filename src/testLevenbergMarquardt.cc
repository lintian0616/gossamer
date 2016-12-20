// Copyright (c) 2008-2016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "LevenbergMarquardt.hh"

#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <cmath>
#include <random>
#include <boost/math/special_functions.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/numeric/ublas/io.hpp>

using namespace boost;
using namespace boost::math;
using namespace boost::numeric::ublas;

#define GOSS_TEST_MODULE TestLevenbergMarquardt
#include "testBegin.hh"

LevenbergMarquardt::vec_t
gaussian(const LevenbergMarquardt::params_t& pParams,
         const LevenbergMarquardt::vec_t& pX)
{
    LevenbergMarquardt::vec_t ys(pX.size());

    double w = pParams[0];
    double mean = pParams[1];
    double stddev = pParams[2];

    if (stddev < 0.0)
    {
        throw LevenbergMarquardt::DomainError();
    }

    boost::math::normal normDist(mean,stddev);

    for (uint64_t i = 0; i < pX.size(); ++i)
    {
        ys[i] = w * pdf(normDist, pX[i]);
    }

    return ys;
}


LevenbergMarquardt::vec_t
kmerModel(const LevenbergMarquardt::params_t& pParams,
          const LevenbergMarquardt::vec_t& pX)
{
    LevenbergMarquardt::vec_t ys(pX.size());

    double mix = pParams[0];
    double lambda = pParams[1];
    double mean = pParams[2];
    double stddev = pParams[3];

    if (stddev < 0.0 || lambda < 0.0)
    {
        throw LevenbergMarquardt::DomainError();
    }

    boost::math::poisson poissDist(lambda);
    boost::math::normal normDist(mean,stddev);

    double massAtZero = mix * pdf(poissDist, 0) + (1.0 - mix) * pdf(normDist, 0);
    double massAtOne = mix * pdf(poissDist, 1) + (1.0 - mix) * pdf(normDist, 1);

    double scale = 1000.0 / (1.0 - massAtZero - massAtOne);

    for (uint64_t i = 0; i < pX.size(); ++i)
    {
        ys[i] = scale * (mix * pdf(poissDist, pX[i])
                         + (1.0 - mix) * pdf(normDist, pX[i]));
    }
    return ys;
}


#if 1
BOOST_AUTO_TEST_CASE(testGaussianFit)
{
    static uint64_t xmin = 1;
    static uint64_t xmax = 50000;
    double w = 100.0;
    double mean = 10.0;
    double stddev = 10.0;

    Logger log(std::cerr);

    vector<double> realParams(3);
    realParams[0] = w;
    realParams[1] = mean;
    realParams[2] = stddev;

    vector<double> xs(xmax - xmin + 1);
    for (uint64_t x = xmin; x <= xmax; ++x)
    {
        xs[x - xmin] = x;
    }
    vector<double> ys = gaussian(realParams, xs);

    std::mt19937 rng(19);
    std::normal_distribution<> randist(0.0, 0.1);

    std::vector<std::pair<double,double> > data;
    double mass = 0.0;
    for (uint64_t i = 0; i < xs.size(); ++i)
    {
        double x = xs[i];
        double y = ys[i] + randist(rng);
        data.push_back(std::pair<double,double>(x, y));
        mass += y;
    }


    std::vector<double> guessParams(3);
    guessParams[0] = 100.0;
    guessParams[1] = 10.0;
    guessParams[2] = 5.0;
    LevenbergMarquardt solver(gaussian, guessParams, data);

    std::vector<double> estParams(3);
    std::vector<double> estStdDev(3);
    double chiSq;
    solver.evaluate(log, estParams, estStdDev, chiSq);

    BOOST_CHECK(estParams[0] - 3 * estStdDev[0] <= realParams[0]);
    BOOST_CHECK(realParams[0] <= estParams[0] + 3 * estStdDev[0]);
    BOOST_CHECK(estParams[1] - 3 * estStdDev[1] <= realParams[1]);
    BOOST_CHECK(realParams[1] <= estParams[1] + 3 * estStdDev[1]);
    BOOST_CHECK(estParams[2] - 3 * estStdDev[2] <= realParams[2]);
    BOOST_CHECK(realParams[2] <= estParams[2] + 3 * estStdDev[2]);

    chi_squared chiSqDist(data.size() - realParams.size());
    BOOST_CHECK_LT(chiSq, quantile(chiSqDist, 0.99));
}
#endif

static uint64_t
sKmerData[][2] = {
    // { 1, 53274392 },
    { 2, 2877282 }, { 3, 496376 }, { 4, 167942 }, { 5, 82786 },
    { 6, 48588 }, { 7, 32696 }, { 8, 22300 }, { 9, 16716 }, { 10, 13008 },
    { 11, 10788 }, { 12, 9002 }, { 13, 7772 }, { 14, 6646 }, { 15, 5464 },
    { 16, 5402 }, { 17, 4816 }, { 18, 4178 }, { 19, 4044 }, { 20, 3892 },
    { 21, 3882 }, { 22, 3924 }, { 23, 4422 }, { 24, 4156 }, { 25, 4198 },
    { 26, 3918 }, { 27, 4074 }, { 28, 4098 }, { 29, 4310 }, { 30, 4808 },
    { 31, 4632 }, { 32, 4894 }, { 33, 5050 }, { 34, 5110 }, { 35, 5200 },
    { 36, 5369 }, { 37, 5868 }, { 38, 5976 }, { 39, 5988 }, { 40, 5788 },
    { 41, 6398 }, { 42, 6516 }, { 43, 6542 }, { 44, 6966 }, { 45, 7064 },
    { 46, 7210 }, { 47, 7322 }, { 48, 7284 }, { 49, 7686 }, { 50, 7772 },
    { 51, 7814 }, { 52, 8212 }, { 53, 8630 }, { 54, 8956 }, { 55, 9220 },
    { 56, 9412 }, { 57, 9804 }, { 58, 10350 }, { 59, 10466 }, { 60, 10760 },
    { 61, 10984 }, { 62, 11598 }, { 63, 11948 }, { 64, 12336 }, { 65, 12420 },
    { 66, 12724 }, { 67, 13038 }, { 68, 13526 }, { 69, 13730 }, { 70, 14300 },
    { 71, 14786 }, { 72, 15178 }, { 73, 15742 }, { 74, 16332 }, { 75, 16592 },
    { 76, 16916 }, { 77, 18304 }, { 78, 18844 }, { 79, 19634 }, { 80, 20120 },
    { 81, 20280 }, { 82, 21022 }, { 83, 20888 }, { 84, 21774 }, { 85, 22490 },
    { 86, 23008 }, { 87, 23922 }, { 88, 24386 }, { 89, 25346 }, { 90, 25906 },
    { 91, 27270 }, { 92, 28268 }, { 93, 28544 }, { 94, 29382 }, { 95, 30354 },
    { 96, 30792 }, { 97, 31438 }, { 98, 32006 }, { 99, 33386 }, { 100, 34346 },
    { 101, 34922 }, { 102, 36206 }, { 103, 36812 }, { 104, 38293 },
    { 105, 38908 }, { 106, 39844 }, { 107, 40872 }, { 108, 41332 },
    { 109, 42564 }, { 110, 43158 }, { 111, 44988 }, { 112, 45482 },
    { 113, 46832 }, { 114, 47514 }, { 115, 48644 }, { 116, 49908 },
    { 117, 52144 }, { 118, 53896 }, { 119, 55864 }, { 120, 58022 },
    { 121, 58960 }, { 122, 61424 }, { 123, 62642 }, { 124, 63666 },
    { 125, 65172 }, { 126, 66798 }, { 127, 68332 }, { 128, 70408 },
    { 129, 72346 }, { 130, 74340 }, { 131, 77226 }, { 132, 77526 },
    { 133, 79842 }, { 134, 82602 }, { 135, 82712 }, { 136, 85562 },
    { 137, 87328 }, { 138, 88464 }, { 139, 89684 }, { 140, 91290 },
    { 141, 92646 }, { 142, 93096 }, { 143, 95494 }, { 144, 95434 },
    { 145, 96106 }, { 146, 97572 }, { 147, 97596 }, { 148, 99410 },
    { 149, 98504 }, { 150, 99806 }, { 151, 99862 }, { 152, 99900 },
    { 153, 99560 }, { 154, 99352 }, { 155, 99520 }, { 156, 98584 },
    { 157, 98610 }, { 158, 98852 }, { 159, 96598 }, { 160, 97306 },
    { 161, 95652 }, { 162, 94974 }, { 163, 94192 }, { 164, 93756 },
    { 165, 91416 }, { 166, 89538 }, { 167, 89236 }, { 168, 87508 },
    { 169, 86010 }, { 170, 84512 }, { 171, 83314 }, { 172, 81222 },
    { 173, 79356 }, { 174, 78802 }, { 175, 76606 }, { 176, 74876 },
    { 177, 73386 }, { 178, 72476 }, { 179, 70564 }, { 180, 68810 },
    { 181, 67810 }, { 182, 66160 }, { 183, 64216 }, { 184, 62060 },
    { 185, 60282 }, { 186, 58486 }, { 187, 56994 }, { 188, 55620 },
    { 189, 53946 }, { 190, 52586 }, { 191, 50126 }, { 192, 49294 },
    { 193, 47168 }, { 194, 46376 }, { 195, 44526 }, { 196, 42512 },
    { 197, 41326 }, { 198, 40592 }, { 199, 39038 }, { 200, 38196 },
    { 201, 37080 }, { 202, 35586 }, { 203, 34372 }, { 204, 32878 },
    { 205, 32056 }, { 206, 30774 }, { 207, 29906 }, { 208, 28798 },
    { 209, 27834 }, { 210, 26350 }, { 211, 25246 }, { 212, 24056 },
    { 213, 22912 }, { 214, 21950 }, { 215, 21488 }, { 216, 20772 },
    { 217, 20084 }, { 218, 19250 }, { 219, 18674 }, { 220, 17552 },
    { 221, 17082 }, { 222, 16338 }, { 223, 15904 }, { 224, 15200 },
    { 225, 14228 }, { 226, 14028 }, { 227, 13324 }, { 228, 12586 },
    { 229, 11966 }, { 230, 11630 }, { 231, 11204 }, { 232, 10370 },
    { 233, 9914 }, { 234, 9534 }, { 235, 9136 }, { 236, 8722 }, { 237, 8154 },
    { 238, 8030 }, { 239, 7452 }, { 240, 7084 }, { 241, 6526 }, { 242, 6574 },
    { 243, 6056 }, { 244, 5730 }, { 245, 5116 }, { 246, 4964 }, { 247, 4618 },
    { 248, 4266 }, { 249, 4116 }, { 250, 3832 }, { 251, 3740 }, { 252, 3644 },
    { 253, 3346 }, { 254, 3226 }, { 255, 3128 }, { 256, 2908 }, { 257, 2712 },
    { 258, 2552 }, { 259, 2494 }, { 260, 2262 }, { 261, 2010 }, { 262, 1776 },
    { 263, 1732 }, { 264, 1622 }, { 265, 1702 }, { 266, 1470 }, { 267, 1474 }
};


static uint64_t
sKmerData2[][2] = {
    // { 1, 199756042 },
    { 2, 24119221 },
    { 3, 6630206 },
    { 4, 2333801 },
    { 5, 1010472 },
    { 6, 527884 },
    { 7, 318342 },
    { 8, 213968 },
    { 9, 152284 },
    { 10, 113820 },
    { 11, 89492 },
    { 12, 70528 },
    { 13, 57146 },
    { 14, 46824 },
    { 15, 39434 },
    { 16, 33191 },
    { 17, 28432 },
    { 18, 24754 },
    { 19, 21300 },
    { 20, 18866 },
    { 21, 16512 },
    { 22, 14824 },
    { 23, 13308 },
    { 24, 11766 },
    { 25, 10760 },
    { 26, 10042 },
    { 27, 9198 },
    { 28, 8031 },
    { 29, 7228 },
    { 30, 6976 },
    { 31, 6286 },
    { 32, 6072 },
    { 33, 5432 },
    { 34, 5204 },
    { 35, 4706 },
    { 36, 4302 },
    { 37, 4112 },
    { 38, 3900 },
    { 39, 3648 },
    { 40, 3572 },
    { 41, 3274 },
    { 42, 3122 },
    { 43, 2870 },
    { 44, 2848 },
    { 45, 2658 },
    { 46, 2530 },
    { 47, 2376 },
    { 48, 2288 },
    { 49, 2296 },
    { 50, 2216 },
    { 51, 2054 },
    { 52, 2068 },
    { 53, 2030 },
    { 54, 1876 },
    { 55, 1968 },
    { 56, 1790 },
    { 57, 1734 },
    { 58, 1668 },
    { 59, 1584 },
    { 60, 1584 },
    { 61, 1492 },
    { 62, 1480 },
    { 63, 1468 },
    { 64, 1370 },
    { 65, 1370 },
    { 66, 1222 },
    { 67, 1250 },
    { 68, 1218 },
    { 69, 1198 },
    { 70, 1180 },
    { 71, 1094 },
    { 72, 1072 },
    { 73, 1042 },
    { 74, 1094 },
    { 75, 914 },
    { 76, 928 },
    { 77, 914 },
    { 78, 964 },
    { 79, 868 },
    { 80, 1010 },
    { 81, 922 },
    { 82, 872 },
    { 83, 898 },
    { 84, 854 },
    { 85, 852 },
    { 86, 747 },
    { 87, 822 },
    { 88, 784 },
    { 89, 842 },
    { 90, 880 },
    { 91, 808 },
    { 92, 860 },
    { 93, 868 },
    { 94, 816 },
    { 95, 850 },
    { 96, 794 },
    { 97, 824 },
    { 98, 888 },
    { 99, 910 },
    { 100, 824 },
    { 101, 812 },
    { 102, 842 },
    { 103, 880 },
    { 104, 864 },
    { 105, 796 },
    { 106, 810 },
    { 107, 810 },
    { 108, 726 },
    { 109, 764 },
    { 110, 776 },
    { 111, 786 },
    { 112, 816 },
    { 113, 846 },
    { 114, 838 },
    { 115, 722 },
    { 116, 806 },
    { 117, 714 },
    { 118, 744 },
    { 119, 816 },
    { 120, 782 },
    { 121, 810 },
    { 122, 800 },
    { 123, 830 },
    { 124, 826 },
    { 125, 868 },
    { 126, 884 },
    { 127, 874 },
    { 128, 874 },
    { 129, 786 },
    { 130, 884 },
    { 131, 816 },
    { 132, 838 },
    { 133, 852 },
    { 134, 846 },
    { 135, 850 },
    { 136, 844 },
    { 137, 854 },
    { 138, 882 },
    { 139, 848 },
    { 140, 938 },
    { 141, 902 },
    { 142, 904 },
    { 143, 1002 },
    { 144, 952 },
    { 145, 884 },
    { 146, 942 },
    { 147, 970 },
    { 148, 1012 },
    { 149, 956 },
    { 150, 1020 },
    { 151, 1098 },
    { 152, 1100 },
    { 153, 994 },
    { 154, 1122 },
    { 155, 988 },
    { 156, 998 },
    { 157, 1192 },
    { 158, 1174 },
    { 159, 1100 },
    { 160, 1134 },
    { 161, 1122 },
    { 162, 1070 },
    { 163, 978 },
    { 164, 1052 },
    { 165, 1146 },
    { 166, 1038 },
    { 167, 1194 },
    { 168, 1144 },
    { 169, 1230 },
    { 170, 1134 },
    { 171, 1144 },
    { 172, 1212 },
    { 173, 1224 },
    { 174, 1200 },
    { 175, 1186 },
    { 176, 1166 },
    { 177, 1298 },
    { 178, 1164 },
    { 179, 1210 },
    { 180, 1362 },
    { 181, 1220 },
    { 182, 1278 },
    { 183, 1276 },
    { 184, 1286 },
    { 185, 1240 },
    { 186, 1348 },
    { 187, 1244 },
    { 188, 1328 },
    { 189, 1394 },
    { 190, 1326 },
    { 191, 1306 },
    { 192, 1316 },
    { 193, 1340 },
    { 194, 1344 },
    { 195, 1348 },
    { 196, 1330 },
    { 197, 1290 },
    { 198, 1418 },
    { 199, 1346 },
    { 200, 1436 },
    { 201, 1418 },
    { 202, 1430 },
    { 203, 1510 },
    { 204, 1576 },
    { 205, 1496 },
    { 206, 1528 },
    { 207, 1540 },
    { 208, 1700 },
    { 209, 1564 },
    { 210, 1528 },
    { 211, 1416 },
    { 212, 1490 },
    { 213, 1600 },
    { 214, 1558 },
    { 215, 1594 },
    { 216, 1622 },
    { 217, 1586 },
    { 218, 1688 },
    { 219, 1634 },
    { 220, 1758 },
    { 221, 1542 },
    { 222, 1662 },
    { 223, 1610 },
    { 224, 1704 },
    { 225, 1698 },
    { 226, 1692 },
    { 227, 1634 },
    { 228, 1662 },
    { 229, 1736 },
    { 230, 1802 },
    { 231, 1770 },
    { 232, 1678 },
    { 233, 1662 },
    { 234, 1746 },
    { 235, 1672 },
    { 236, 1792 },
    { 237, 1754 },
    { 238, 1810 },
    { 239, 1690 },
    { 240, 1864 },
    { 241, 1880 },
    { 242, 1754 },
    { 243, 1914 },
    { 244, 1898 },
    { 245, 1954 },
    { 246, 1916 },
    { 247, 1908 },
    { 248, 1878 },
    { 249, 1986 },
    { 250, 1938 },
    { 251, 1958 },
    { 252, 1926 },
    { 253, 1888 },
    { 254, 2016 },
    { 255, 1910 },
    { 256, 1892 },
    { 257, 1898 },
    { 258, 1900 },
    { 259, 1856 },
    { 260, 2006 },
    { 261, 2036 },
    { 262, 2084 },
    { 263, 1950 },
    { 264, 2132 },
    { 265, 2152 },
    { 266, 2086 },
    { 267, 2140 },
    { 268, 2214 },
    { 269, 2022 },
    { 270, 2138 },
    { 271, 2212 },
    { 272, 2074 },
    { 273, 2150 },
    { 274, 2134 },
    { 275, 2170 },
    { 276, 2106 },
    { 277, 2096 },
    { 278, 2274 },
    { 279, 2192 },
    { 280, 2100 },
    { 281, 2214 },
    { 282, 2260 },
    { 283, 2118 },
    { 284, 2332 },
    { 285, 2170 },
    { 286, 2352 },
    { 287, 2214 },
    { 288, 2348 },
    { 289, 2406 },
    { 290, 2414 },
    { 291, 2320 },
    { 292, 2346 },
    { 293, 2574 },
    { 294, 2476 },
    { 295, 2546 },
    { 296, 2440 },
    { 297, 2520 },
    { 298, 2432 },
    { 299, 2518 },
    { 300, 2616 },
    { 301, 2464 },
    { 302, 2670 },
    { 303, 2522 },
    { 304, 2638 },
    { 305, 2688 },
    { 306, 2646 },
    { 307, 2478 },
    { 308, 2570 },
    { 309, 2554 },
    { 310, 2584 },
    { 311, 2600 },
    { 312, 2628 },
    { 313, 2526 },
    { 314, 2778 },
    { 315, 2586 },
    { 316, 2778 },
    { 317, 2804 },
    { 318, 2624 },
    { 319, 2762 },
    { 320, 2668 },
    { 321, 2762 },
    { 322, 2854 },
    { 323, 2760 },
    { 324, 2890 },
    { 325, 2984 },
    { 326, 2770 },
    { 327, 2868 },
    { 328, 2960 },
    { 329, 3092 },
    { 330, 3052 },
    { 331, 3118 },
    { 332, 3124 },
    { 333, 3034 },
    { 334, 3140 },
    { 335, 3140 },
    { 336, 3380 },
    { 337, 3226 },
    { 338, 3224 },
    { 339, 3348 },
    { 340, 3266 },
    { 341, 3356 },
    { 342, 3360 },
    { 343, 3398 },
    { 344, 3488 },
    { 345, 3534 },
    { 346, 3304 },
    { 347, 3612 },
    { 348, 3480 },
    { 349, 3632 },
    { 350, 3520 },
    { 351, 3628 },
    { 352, 3536 },
    { 353, 3582 },
    { 354, 3554 },
    { 355, 3548 },
    { 356, 3616 },
    { 357, 3734 },
    { 358, 3618 },
    { 359, 3770 },
    { 360, 3726 },
    { 361, 3764 },
    { 362, 3696 },
    { 363, 3708 },
    { 364, 3736 },
    { 365, 3798 },
    { 366, 3668 },
    { 367, 3756 },
    { 368, 3908 },
    { 369, 4102 },
    { 370, 4134 },
    { 371, 4088 },
    { 372, 4162 },
    { 373, 4196 },
    { 374, 4052 },
    { 375, 4230 },
    { 376, 4414 },
    { 377, 4330 },
    { 378, 4134 },
    { 379, 4488 },
    { 380, 4514 },
    { 381, 4506 },
    { 382, 4704 },
    { 383, 4488 },
    { 384, 4392 },
    { 385, 4510 },
    { 386, 4812 },
    { 387, 4466 },
    { 388, 4536 },
    { 389, 4674 },
    { 390, 4824 },
    { 391, 4768 },
    { 392, 5050 },
    { 393, 4830 },
    { 394, 4812 },
    { 395, 5086 },
    { 396, 4880 },
    { 397, 5026 },
    { 398, 5036 },
    { 399, 5126 },
    { 400, 4878 },
    { 401, 5008 },
    { 402, 5440 },
    { 403, 5308 },
    { 404, 5202 },
    { 405, 5206 },
    { 406, 5275 },
    { 407, 5208 },
    { 408, 5144 },
    { 409, 5318 },
    { 410, 5228 },
    { 411, 5240 },
    { 412, 5372 },
    { 413, 5542 },
    { 414, 5386 },
    { 415, 5664 },
    { 416, 5604 },
    { 417, 5570 },
    { 418, 5694 },
    { 419, 5546 },
    { 420, 5692 },
    { 421, 5582 },
    { 422, 5632 },
    { 423, 5748 },
    { 424, 5562 },
    { 425, 5622 },
    { 426, 5834 },
    { 427, 5628 },
    { 428, 5762 },
    { 429, 5802 },
    { 430, 5666 },
    { 431, 5666 },
    { 432, 5964 },
    { 433, 5850 },
    { 434, 5962 },
    { 435, 6116 },
    { 436, 6068 },
    { 437, 5924 },
    { 438, 5912 },
    { 439, 5888 },
    { 440, 6300 },
    { 441, 6116 },
    { 442, 6504 },
    { 443, 6356 },
    { 444, 6528 },
    { 445, 6442 },
    { 446, 6594 },
    { 447, 6680 },
    { 448, 6908 },
    { 449, 6860 },
    { 450, 6964 },
    { 451, 6758 },
    { 452, 6992 },
    { 453, 6948 },
    { 454, 6856 },
    { 455, 6888 },
    { 456, 7140 },
    { 457, 6908 },
    { 458, 7088 },
    { 459, 6988 },
    { 460, 7134 },
    { 461, 7320 },
    { 462, 7280 },
    { 463, 7352 },
    { 464, 7288 },
    { 465, 7374 },
    { 466, 7452 },
    { 467, 7530 },
    { 468, 7282 },
    { 469, 7476 },
    { 470, 7620 },
    { 471, 7666 },
    { 472, 7614 },
    { 473, 7702 },
    { 474, 7848 },
    { 475, 7618 },
    { 476, 7664 },
    { 477, 7540 },
    { 478, 7686 },
    { 479, 7730 },
    { 480, 7642 },
    { 481, 7772 },
    { 482, 7896 },
    { 483, 7856 },
    { 484, 7896 },
    { 485, 8084 },
    { 486, 7932 },
    { 487, 7758 },
    { 488, 8354 },
    { 489, 8254 },
    { 490, 8218 },
    { 491, 8220 },
    { 492, 8318 },
    { 493, 8670 },
    { 494, 8416 },
    { 495, 8496 },
    { 496, 8290 },
    { 497, 8408 },
    { 498, 8174 },
    { 499, 8548 },
    { 500, 8648 },
    { 501, 8634 },
    { 502, 8626 },
    { 503, 8522 },
    { 504, 8736 },
    { 505, 8412 },
    { 506, 8710 },
    { 507, 8684 },
    { 508, 8486 },
    { 509, 8714 },
    { 510, 8764 },
    { 511, 8852 },
    { 512, 8818 },
    { 513, 8820 },
    { 514, 8962 },
    { 515, 8920 },
    { 516, 8798 },
    { 517, 9064 },
    { 518, 9180 },
    { 519, 9098 },
    { 520, 9276 },
    { 521, 9308 },
    { 522, 9446 },
    { 523, 9126 },
    { 524, 9412 },
    { 525, 9652 },
    { 526, 9582 },
    { 527, 9856 },
    { 528, 10004 },
    { 529, 9928 },
    { 530, 9910 },
    { 531, 10028 },
    { 532, 9924 },
    { 533, 10292 },
    { 534, 10152 },
    { 535, 10308 },
    { 536, 10370 },
    { 537, 10654 },
    { 538, 10412 },
    { 539, 10580 },
    { 540, 10460 },
    { 541, 10744 },
    { 542, 10816 },
    { 543, 10618 },
    { 576, 13128 },
    { 577, 13138 },
    { 578, 12924 },
    { 579, 13242 },
    { 580, 13702 },
    { 581, 13514 },
    { 582, 13702 },
    { 583, 13762 },
    { 584, 14196 },
    { 585, 13980 },
    { 586, 14232 },
    { 587, 13856 },
    { 588, 13968 },
    { 589, 14448 },
    { 590, 14396 },
    { 591, 14568 },
    { 592, 14758 },
    { 593, 14502 },
    { 594, 14912 },
    { 595, 14680 },
    { 596, 15096 },
    { 597, 14986 },
    { 598, 15096 },
    { 599, 15328 },
    { 600, 15376 },
    { 601, 15538 },
    { 602, 15750 },
    { 603, 15898 },
    { 604, 16198 },
    { 605, 15748 },
    { 606, 16308 },
    { 607, 16358 },
    { 608, 16330 },
    { 609, 16456 },
    { 610, 16606 },
    { 611, 16482 },
    { 612, 16902 },
    { 613, 16806 },
    { 614, 16960 },
    { 615, 17022 },
    { 616, 17306 },
    { 617, 17578 },
    { 618, 17706 },
    { 619, 17742 },
    { 620, 17548 },
    { 621, 17656 },
    { 622, 17864 },
    { 623, 17686 },
    { 624, 17986 },
    { 625, 18264 },
    { 626, 18064 },
    { 627, 18216 },
    { 628, 17936 },
    { 629, 18376 },
    { 630, 18436 },
    { 631, 18726 },
    { 632, 18694 },
    { 633, 18794 },
    { 634, 18818 },
    { 635, 19170 },
    { 636, 19142 },
    { 637, 19016 },
    { 638, 19706 },
    { 639, 19788 },
    { 640, 19448 },
    { 641, 20074 },
    { 642, 19720 },
    { 643, 19898 },
    { 644, 19700 },
    { 645, 20392 },
    { 646, 20336 },
    { 647, 20072 },
    { 648, 20210 },
    { 649, 20114 },
    { 650, 20388 },
    { 651, 20240 },
    { 652, 20302 },
    { 653, 20540 },
    { 654, 20610 },
    { 655, 20772 },
    { 656, 20818 },
    { 657, 21054 },
    { 658, 21392 },
    { 659, 20682 },
    { 660, 20816 },
    { 661, 21478 },
    { 662, 21330 },
    { 663, 21748 },
    { 664, 21732 },
    { 665, 21720 },
    { 666, 22106 },
    { 667, 22228 },
    { 668, 21846 },
    { 669, 22130 },
    { 670, 22128 },
    { 671, 22080 },
    { 672, 22650 },
    { 673, 22364 },
    { 674, 22292 },
    { 675, 22384 },
    { 676, 22462 },
    { 677, 22396 },
    { 678, 22570 },
    { 679, 22518 },
    { 680, 22450 },
    { 681, 23000 },
    { 682, 22994 },
    { 683, 22912 },
    { 684, 22816 },
    { 685, 23166 },
    { 686, 22852 },
    { 687, 23096 },
    { 688, 23256 },
    { 689, 22868 },
    { 690, 23306 },
    { 691, 23464 },
    { 692, 23138 },
    { 693, 23158 },
    { 694, 23298 },
    { 695, 23474 },
    { 696, 23610 },
    { 697, 23074 },
    { 698, 23270 },
    { 699, 23360 },
    { 700, 23424 },
    { 701, 23044 },
    { 702, 23272 },
    { 703, 23166 },
    { 704, 23112 },
    { 705, 23128 },
    { 706, 23482 },
    { 707, 23214 },
    { 708, 22820 },
    { 709, 22968 },
    { 710, 23216 },
    { 711, 23446 },
    { 712, 22872 },
    { 713, 23000 },
    { 714, 23428 },
    { 715, 22862 },
    { 716, 23322 },
    { 717, 22962 },
    { 718, 23040 },
    { 719, 23288 },
    { 720, 23200 },
    { 721, 23116 },
    { 722, 23456 },
    { 723, 23256 },
    { 724, 23156 },
    { 725, 23182 },
    { 726, 23096 },
    { 727, 23198 },
    { 728, 23028 },
    { 729, 23018 },
    { 730, 23272 },
    { 731, 23186 },
    { 732, 23080 },
    { 733, 22862 },
    { 734, 22972 },
    { 735, 22698 },
    { 736, 22950 },
    { 737, 22444 },
    { 738, 22680 },
    { 739, 22638 },
    { 740, 22416 },
    { 741, 22506 },
    { 742, 22976 },
    { 743, 22200 },
    { 744, 22588 },
    { 745, 22916 },
    { 746, 22730 },
    { 747, 22338 },
    { 748, 22580 },
    { 749, 22670 },
    { 750, 22092 },
    { 751, 22564 },
    { 752, 22150 },
    { 753, 22026 },
    { 754, 21800 },
    { 755, 21876 },
    { 756, 21954 },
    { 757, 21778 },
    { 758, 21550 },
    { 759, 22156 },
    { 760, 21618 },
    { 761, 21226 },
    { 762, 21398 },
    { 763, 21320 },
    { 764, 21678 },
    { 765, 21174 },
    { 766, 21556 },
    { 767, 21454 },
    { 768, 21360 },
    { 769, 21154 },
    { 770, 20754 },
    { 771, 20956 },
    { 772, 20724 },
    { 773, 20894 },
    { 774, 20574 },
    { 775, 20302 },
    { 776, 20324 },
    { 777, 20056 },
    { 778, 20274 },
    { 779, 19862 },
    { 780, 20274 },
    { 781, 20046 },
    { 782, 19810 },
    { 783, 19716 },
    { 784, 19318 },
    { 785, 19160 },
    { 786, 19438 },
    { 787, 19248 },
    { 788, 18882 },
    { 789, 19044 },
    { 790, 18916 },
    { 791, 18904 },
    { 792, 18546 },
    { 793, 18472 },
    { 794, 18468 },
    { 795, 18328 },
    { 796, 18502 },
    { 797, 18000 },
    { 798, 17722 },
    { 799, 17818 },
    { 800, 18204 },
    { 801, 18244 },
    { 802, 17386 },
    { 803, 17602 },
    { 804, 17550 },
    { 805, 17636 },
    { 806, 17592 },
    { 807, 17086 },
    { 808, 17094 },
    { 809, 17014 },
    { 810, 17158 },
    { 811, 17112 },
    { 812, 16612 },
    { 813, 16802 },
    { 814, 16538 },
    { 815, 16438 },
    { 816, 16096 },
    { 817, 16448 },
    { 818, 16590 },
    { 819, 15960 },
    { 820, 16140 },
    { 821, 16238 },
    { 822, 15868 },
    { 823, 15908 },
    { 824, 15652 },
    { 825, 15860 },
    { 826, 15488 },
    { 827, 15920 },
    { 828, 15756 },
    { 829, 15044 },
    { 830, 15286 },
    { 831, 14978 },
    { 832, 15148 },
    { 833, 14820 },
    { 834, 14754 },
    { 835, 14662 },
    { 836, 14454 },
    { 837, 14260 },
    { 838, 14712 },
    { 839, 14518 },
    { 840, 14108 },
    { 841, 14114 },
    { 842, 14168 },
    { 843, 14156 },
    { 844, 13964 },
    { 845, 14198 },
    { 846, 13648 },
    { 847, 13496 },
    { 848, 13776 },
    { 849, 13594 },
    { 850, 13488 },
    { 851, 13106 },
    { 852, 13378 },
    { 853, 12966 },
    { 854, 13110 },
    { 855, 12946 },
    { 856, 13048 },
    { 857, 12858 },
    { 858, 12650 },
    { 859, 12500 },
    { 860, 12422 },
    { 861, 12660 },
    { 862, 12276 },
    { 863, 12432 },
    { 864, 12162 },
    { 865, 12328 },
    { 866, 11848 },
    { 867, 11910 },
    { 868, 11946 },
    { 869, 11920 },
    { 870, 12144 },
    { 871, 11656 },
    { 872, 11602 },
    { 873, 11588 },
    { 874, 11676 },
    { 875, 11474 },
    { 876, 11622 },
    { 877, 11552 },
    { 878, 11204 },
    { 879, 11458 },
    { 880, 11074 },
    { 881, 11150 },
    { 882, 11134 },
    { 883, 11014 },
    { 884, 10780 },
    { 885, 10572 },
    { 886, 10590 },
    { 887, 10536 },
    { 888, 10346 },
    { 889, 10512 },
    { 890, 10188 },
    { 891, 10164 },
    { 892, 10274 },
    { 893, 9928 },
    { 894, 10054 },
    { 895, 10010 },
    { 896, 9712 },
    { 897, 9660 },
    { 898, 9964 },
    { 899, 9732 },
    { 900, 9656 },
    { 901, 9260 },
    { 902, 9296 },
    { 903, 9504 },
    { 904, 9300 },
    { 905, 9128 },
    { 906, 9030 },
    { 907, 9200 },
    { 908, 8990 },
    { 909, 9086 },
    { 910, 8586 },
    { 911, 8810 },
    { 912, 8818 },
    { 913, 8694 },
    { 914, 8402 },
    { 915, 8382 },
    { 916, 8518 },
    { 917, 8194 },
    { 918, 8164 },
    { 919, 7966 },
    { 920, 7992 },
    { 921, 8044 },
    { 922, 7814 },
    { 923, 7982 },
    { 924, 7852 },
    { 925, 8008 },
    { 926, 7938 },
    { 927, 7574 },
    { 928, 7734 },
    { 929, 7608 },
    { 930, 7506 },
    { 931, 7372 },
    { 932, 7350 },
    { 933, 7368 },
    { 934, 7484 },
    { 935, 7262 },
    { 936, 7204 },
    { 937, 7302 },
    { 938, 6984 },
    { 939, 6976 },
    { 940, 7064 },
    { 941, 7078 },
    { 942, 6998 },
    { 943, 6902 },
    { 944, 6968 },
    { 945, 6648 },
    { 946, 6636 },
    { 947, 6536 },
    { 948, 6266 },
    { 949, 6572 },
    { 950, 6452 },
    { 951, 6264 },
    { 952, 6142 },
    { 953, 6192 },
    { 954, 6182 },
    { 955, 6038 },
    { 956, 6128 },
    { 957, 5792 },
    { 958, 5958 },
    { 959, 5760 },
    { 960, 5850 },
    { 961, 5672 },
    { 962, 5722 },
    { 963, 5614 },
    { 964, 5644 },
    { 965, 5494 },
    { 966, 5578 },
    { 967, 5438 },
    { 968, 5460 },
    { 969, 5236 },
    { 970, 5342 },
    { 971, 5186 },
    { 972, 5254 },
    { 973, 5188 },
    { 974, 5076 },
    { 975, 5056 },
    { 976, 4828 },
    { 977, 5108 },
    { 978, 4974 },
    { 979, 4882 },
    { 980, 4948 },
    { 981, 4664 },
    { 982, 4942 },
    { 983, 4718 },
    { 984, 4688 },
    { 985, 4766 },
    { 986, 4492 },
    { 987, 4600 },
    { 988, 4522 },
    { 989, 4366 },
    { 990, 4606 },
    { 991, 4198 },
    { 992, 4238 },
    { 993, 4422 },
    { 994, 4368 },
    { 995, 4524 },
    { 996, 4292 },
    { 997, 4210 },
    { 998, 4348 },
    { 999, 4210 },
    { 1000, 4114 }
};


#if 1
BOOST_AUTO_TEST_CASE(testSimulatedKmerModelFit)
{
    double mix = 0.86;
    double lambda = 0.95;
    double mean = 100.0;
    double stddev = 20.0;

    Logger log(std::cerr);

    vector<double> realParams(4);
    realParams[0] = mix;
    realParams[1] = lambda;
    realParams[2] = mean;
    realParams[3] = stddev;

    vector<double> xs(500);
    for (uint64_t x = 0; x < xs.size(); ++x)
    {
        xs[x] = x + 1;
    }
    vector<double> ys = kmerModel(realParams, xs);

    std::mt19937 rng(19);
    std::normal_distribution<> randist(0.0, 1.0);

    std::vector<std::pair<double,double> > data;
    for (uint64_t i = 0; i < xs.size(); ++i)
    {
        data.push_back(std::pair<double,double>(xs[i], ys[i] + randist(rng)));
    }

    std::vector<double> guessParams(4);
    guessParams[0] = 0.5;
    guessParams[1] = 1.0;
    guessParams[2] = 200.0;
    guessParams[3] = 50.0;
    LevenbergMarquardt solver(kmerModel, guessParams, data);

    std::vector<double> estParams(4);
    std::vector<double> estStdDev(4);
    double chiSq;
    solver.evaluate(log, estParams, estStdDev, chiSq);

    for (uint64_t i = 0; i < estParams.size(); ++i)
    {
        BOOST_CHECK(estParams[i] - 3 * estStdDev[i] <= realParams[i]);
        BOOST_CHECK(realParams[i] <= estParams[i] + 3 * estStdDev[i]);
    }

    chi_squared chiSqDist(data.size() - estParams.size());
    BOOST_CHECK(chiSq < quantile(chiSqDist, 0.99));
}
#endif

#if 1
BOOST_AUTO_TEST_CASE(testRealKmerModelFit)
{
    typedef std::vector<std::pair<double,double> > data_vector_t;

    data_vector_t data;

    Logger log(std::cerr);

    uint64_t totalKmerCount = 0;
    uint64_t maxKmerCount = 0;
    for (uint64_t i = 0; i < sizeof(sKmerData) / sizeof(sKmerData[0]); ++i)
    {
        data.push_back(std::pair<double,double>(
            sKmerData[i][0], sKmerData[i][1]));
        uint64_t kc = sKmerData[i][1];
        totalKmerCount += kc;
        if (kc > maxKmerCount)
        {
            maxKmerCount = kc;
        }
    }

    double scale = totalKmerCount / 1000.0 / 0.999;

    for (data_vector_t::iterator i = data.begin(); i != data.end(); ++i)
    {
        i->second /= scale;
    }

    std::vector<double> guessParams(4);
    guessParams[0] = 0.8;
    guessParams[1] = 0.95;
    guessParams[2] = 150.0;
    guessParams[3] = 20.0;
    LevenbergMarquardt solver(kmerModel, guessParams, data);

    std::vector<double> estParams(4);
    std::vector<double> estStdDev(4);
    double chiSq;
    solver.evaluate(log, estParams, estStdDev, chiSq);

#if 1
    std::cerr << "Scale = " << scale << '\n';
    std::cerr << "Est mix = " << estParams[0] << " +/- " << estStdDev[0] << "\n";
    std::cerr << "Est lambda = " << estParams[1] << " +/- " << estStdDev[1] << "\n";
    std::cerr << "Est mean = " << estParams[2] << " +/- " << estStdDev[2] << "\n";
    std::cerr << "Est stddev = " << estParams[3] << " +/- " << estStdDev[3] << "\n";
    std::cerr << "chiSq = " << chiSq << "\n";
#endif

    chi_squared chiSqDist(data.size() - estParams.size());
    BOOST_CHECK(chiSq < quantile(chiSqDist, 0.99));

}
#endif

#if 1
BOOST_AUTO_TEST_CASE(testRealKmerModelFit2)
{
    typedef std::vector<std::pair<double,double> > data_vector_t;

    Logger log(std::cerr);

    data_vector_t data;

    uint64_t totalKmerCount = 0;
    uint64_t maxKmerCount = 0;
    for (uint64_t i = 0; i < sizeof(sKmerData2) / sizeof(sKmerData2[0]); ++i)
    {
        data.push_back(std::pair<double,double>(
            sKmerData2[i][0], sKmerData2[i][1]));
        uint64_t kc = sKmerData2[i][1];
        totalKmerCount += kc;
        if (kc > maxKmerCount)
        {
            maxKmerCount = kc;
        }
    }

    double scale = totalKmerCount / 1000.0 / 0.999;

    for (data_vector_t::iterator i = data.begin(); i != data.end(); ++i)
    {
        i->second /= scale;
    }

    std::vector<double> guessParams(4);
    guessParams[0] = 0.9;
    guessParams[1] = 0.1;
    guessParams[2] = 700.0;
    guessParams[3] = 10.0;
    LevenbergMarquardt solver(kmerModel, guessParams, data);

    std::vector<double> estParams(4);
    std::vector<double> estStdDev(4);
    double chiSq;
    solver.evaluate(log, estParams, estStdDev, chiSq);

#if 1
    std::cerr << "Scale = " << scale << '\n';
    std::cerr << "Est mix = " << estParams[0] << " +/- " << estStdDev[0] << "\n";
    std::cerr << "Est lambda = " << estParams[1] << " +/- " << estStdDev[1] << "\n";
    std::cerr << "Est mean = " << estParams[2] << " +/- " << estStdDev[2] << "\n";
    std::cerr << "Est stddev = " << estParams[3] << " +/- " << estStdDev[3] << "\n";
    std::cerr << "chiSq = " << chiSq << "\n";
#endif

    chi_squared chiSqDist(data.size() - estParams.size());
    BOOST_CHECK(chiSq < quantile(chiSqDist, 0.99));

}
#endif



#include "testEnd.hh"


