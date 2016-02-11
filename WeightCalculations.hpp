/*
 * WeightCalculations.hpp
 *
 *  Created on: Jan 23, 2016
 *      Author: Michael
 */

#ifndef WEIGHTCALCULATIONS_HPP_
#define WEIGHTCALCULATIONS_HPP_

#include <assert.h>
#include <algorithm>
#include <vector>

//Let S be a sorted multiset of 5 elements consisting of integers 1-6
//S has 32 subsets, not all neccessarily distinct.
//The weight is the multiplicity of each unique subset of S
void find_num_ways_get_superset_from_subset(int weights[252][32]);

//Returns all the 5-element multisets S consisting of the numbers 1-6 for which "subset" is a subset of S
vector<vector<int>> get_supersets(int subset [5]);

//Returns all the subset of the 5-element set "superset"
vector<vector<int>> get_subsets(int superset [5]);


using namespace std;

#endif /* WEIGHTCALCULATIONS_HPP_ */
