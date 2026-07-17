#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MAIN
int data_angle[] = {
    2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 42, 98, 161, 225, 287, 348, 407, 463, 518, 571, 621, 670, 715, 760, 806, 852, 897, 939, 979, 1019, 1059, 1099, 1138, 1175, 1212, 1248, 1284, 1317, 1349, 1381, 1413, 1444, 1475, 1505, 1534, 1561, 1588, 1614, 1640, 1658, 1675, 1692, 1708, 1710, 1714, 1718, 1722, 1724, 1725, 1725, 1725, 1725, 1725, 1726, 1726};
int speed[] = {
    0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 27, 65, 97, 118, 124, 124, 121, 117, 113, 109, 105, 101, 96, 92, 91, 91, 91, 88, 84, 81, 80, 80, 79, 77, 75, 73, 72, 69, 67, 65, 64, 63, 62, 61, 59, 57, 55, 53, 52, 45, 40, 36, 33, 21, 14, 9, 7, 6, 4, 2, 0, 0, 0, 0};
int buffer[10240];
#endif

/*寻找下降系曲线斜率最大的地方*/
int curve_get_max_down_spot(int *data, int len)
{
#define LEN 2
    int acc_max = 0;
    int spot = 0;
    int count = 0;
    for (int i = LEN; i < (len - LEN); i++)
    {
        if (data[i - LEN] > data[i + LEN])
        {
            if (data[i - LEN] - data[i + LEN] > acc_max)
            {
                spot = i - (LEN + 1);
                acc_max = data[i - LEN] - data[i + LEN];
            }
        }
    }
    return spot;
}

/*寻找曲线的反转点*/
int curve_get_start_spot(int *data, int len)
{
    int Count = 0;
    for (int i = 0; i < (len - 1); i++)
    {
        if (data[i + 1] >= data[i])
        {
            Count = 0;
        }
        else
        {
            Count++;
            if (Count > 3)
                return i - 3;
        }
    }
    return 0;
}

/*求曲线最高点和最低点的差*/
int curve_max_difference(int *data, int len)
{
    int max_num = 0;
    int min_num = 0xfffffff;
    for (int i = 0; i < len; i++)
    {
        if (data[i] > max_num)
            max_num = data[i];
        if (data[i] < min_num)
            min_num = data[i];
    }
    return max_num - min_num;
}

// 计算斜率的标准差(曲线的平滑度,返回值越小越平滑)
float curve_compute_Slope(int *y, int size)
{
    if (size < 2)
    {
        return NAN; // 数据点不足，无法计算斜率
    }

    float sumSlope = 0.0;
    for (int i = 1; i < size; i++)
    {
        float slope = (y[i - 1] - y[i]);
        if (!isinf(slope))
        { // 跳过无穷大的斜率
            sumSlope += slope;
        }
    }

    float meanSlope = sumSlope / (size - 1);

    float sumDeviationSquared = 0.0;
    for (int i = 1; i < size; i++)
    {
        float slope = (y[i - 1] - y[i]);
        if (!isinf(slope))
        {
            sumDeviationSquared += pow(slope - meanSlope, 2);
        }
    }

    float variance = sumDeviationSquared / (size - 2); // 使用n-2来避免除以零和样本偏差
    return sqrt(variance);                             // 返回标准差
}

#ifdef MAIN
int main()
{
    float min = 1000;
    float min2 = 1000;
    memset(buffer, 0, sizeof(buffer));

    float big = 1;
    int count = 0;
    float min_big = 1;
    float min_big2 = 1;

    int data = 0;
    while (1)
    {
        for (int i = 0; i < sizeof(data_angle) / sizeof(data_angle[0]); i++)
        {
            buffer[i] = speed[i] + (data_angle[i] * big);
        }

        int start_spot = curve_get_start_spot(speed, sizeof(speed) / sizeof(speed[0]));
        int end_spot = curve_get_max_down_spot(speed, sizeof(speed) / sizeof(speed[0]));
        data = curve_max_difference(&buffer[start_spot], end_spot - start_spot);

        if (data <= min)
        {
            min_big = big;
            min = data;
        }
        big -= 0.0001;
        if (big < 0.0001)
            break;

        float slopeStdDev = curve_compute_Slope(&buffer[start_spot], end_spot - start_spot - start_spot);
        if (!isnan(slopeStdDev))
        {
            if (slopeStdDev <= min2)
            {
                min_big2 = big;
                min2 = slopeStdDev;
            }
        }

        // printf("big:%f data:%d--------------%f\r\n", big, data, slopeStdDev);
    }
    printf("minD:%f minB:%f\r\n", min, min_big);
    printf("minD:%f minB:%f\r\n", min2, min_big2);
    printf("acc max:%d\r\n", get_max_down_acc(speed, sizeof(speed) / sizeof(speed[0])));
    printf("start:%d\r\n", get_start(speed, sizeof(speed) / sizeof(speed[0])));

    return 0;
}
#endif
