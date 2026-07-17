/*
 * @Author: XinXin
 * @Date: 2024-04-23 01:19:09
 * @LastEditors: XinXin
 * @LastEditTime: 2024-04-25 10:52:23
 * @FilePath: \source\app\math\fitted_curve.h
 * @Description:
 */
#ifndef __FITTED_CURVE_H__
#define __FITTED_CURVE_H__

int curve_get_max_down_spot(int *data, int len);
int curve_get_start_spot(int *data, int len);
int curve_max_difference(int *data, int len);
float curve_compute_Slope(int *y, int size);

#endif
