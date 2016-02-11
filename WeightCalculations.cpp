#include "WeightCalculations.hpp"

//A few combinatorial helper functions

vector<vector<int>> get_subsets(int superset [5])
{

	vector<vector<int>> result_with_duplicates (32);
	for (int i=0; i<32; i++)
	{
		for (int j=0; j<5; j++)
			if (i & (1 << j))
				result_with_duplicates[i].push_back(superset[j]);
	}

	//return result_with_duplicates;
	vector<vector<int>> result;

	sort(result_with_duplicates.begin(), result_with_duplicates.end());

	for (int i=0; i<32; i++)
		if (!binary_search(result.begin(), result.end(), result_with_duplicates[i]))
			result.push_back(result_with_duplicates[i]);

	return result;

}


vector<vector<int>> get_superset_recursion(vector<vector<int>> &result, int num_zeros, int subset [5], \
		int recursion_index, vector<int> superset_so_far)
{
	static int first_fixed_ele;
	if (recursion_index == 0)
	{
		for (int i=num_zeros; i<5; i++)
			superset_so_far[i] = subset[i];
		first_fixed_ele = (num_zeros == 5) ? 6 : subset[num_zeros];
	}

	if (recursion_index < num_zeros)
	{
		for (superset_so_far[recursion_index] = ((recursion_index == 0) ? 1 : superset_so_far[recursion_index - 1]); \
		superset_so_far[recursion_index] <= first_fixed_ele; superset_so_far[recursion_index]++)
			get_superset_recursion(result, num_zeros, subset, recursion_index + 1, superset_so_far);
	}
	else
		result.push_back(superset_so_far);
	//return result;
	if (recursion_index == 0)
		return result;
	else
	{
		static vector<vector<int>> empty (0);
		return empty;
	}
}

vector<vector<int>> get_supersets()
{
	int subset [5] = {0};
	int num_zeros = 0;
	while (subset[num_zeros] == 0)
	{
		num_zeros++;
		if (num_zeros == 5)
			break;
	}
	vector<vector<int>> result;
	return get_superset_recursion(result, num_zeros, subset, 0, vector<int> (5));
}

int compute_weight(int superset [5], int subset[])
{
	//gets number of elements in subset
	int begin_subset = 0;
	while (!subset[begin_subset])
	{
		begin_subset ++;
		if (begin_subset == 5)
			break;
	}

	//find complement
	vector<int> complement (10);

	auto it = set_difference(superset, superset + 5, subset + begin_subset, subset + 5, complement.begin());
	complement.resize(it - complement.begin());
	//assert(complement.size() == begin_subset);


	int weight = factorial_arr[complement.size()];




	//get num_occurence array
	int num_occ[7] = {0};
	for (unsigned int i = 0; i<complement.size(); i++)
		num_occ[complement[i]] ++;



	//finally, compute weight by looking at repetition
	for (int i=1; i<=6; i++)
		weight /= factorial_arr[num_occ[i]];


	return weight;

}


void find_num_ways_get_superset_from_subset(int weights[252][32])
{
	int row = 0, col = 0;
	vector<vector<int>> supersets = get_supersets();
	for (vector<int> superset : supersets)
	{
		col = 0;
		vector<vector<int>> subsets = get_subsets(&(superset[0]));
		for (vector<int> subset : subsets)
		{
			int subset_arr[5] = {0};
			if (subset.size())
				memcpy(subset_arr + (5 - subset.size()), &(subset[0]), subset.size() * sizeof(int));

			weights[row][col] = compute_weight(&(superset[0]), subset_arr);
			col ++;
		}
		row ++;
	}
}
