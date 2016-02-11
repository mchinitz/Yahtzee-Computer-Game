//#include "YazeeDistributions.hpp"
#include <assert.h>
#include <stdlib.h>

//TODO organize

//part 3

//Convenient to have a couple global variables, since these variables are used throughout the program
//in both distribution calculations and in adjustment calculations.
storing_considered_rolls *find_redundency[8192] = {NULL};
part_three_predicted_scores part_three_arr[8192][13];

int *index_correspondence_subsets(vector<int> &elements, int path_len, bool is_reversing = false);


//Given a 32-bit integer x, returns the number of bits that are set
int num_ones(int x)
{
	int result = 0;
	for (int i=0; i<32; i++)
		if (x & (1 << i))
			result ++;
	return result;
}

//Comparison function for sorting integer arrays such that integers with more bits set are ordered second.
//If they have the same number, small integers are ordered first.
int compare_num_ones(const void *p, const void *q)
{
	int x = *(int *)(p);
	int y = *(int *)(q);
	int num_ones_x = num_ones(x);
	int num_ones_y = num_ones(y);
	if (num_ones_x != num_ones_y)
		return num_ones_x - num_ones_y;
	else
		return x - y;
}

//This function attempts to get around the issue that because of the 35-point bonus, subproblems of smaller length
//actually do depend on the history. We estimate where we expect the upper score to be later on in the game.
//The estimate is likely optimistic, but designed to make the bonus attractive.
int compute_heuristic_increment(bool initial_options[14], bool available_options[14], int initial_upper_score, \
		int initial_num_options, int initial_num_uppers, int removed_ele, int curr_num_uppers, int curr_num_lowers)
{

	//First, assume get 3 of each upper category in complement of available options wrt initial options
	int increment = 0;
	int reduced_initial_num_uppers = initial_num_uppers;
	for (int i=1; i<=6; i++)
		if (initial_options[i])
		{
			if (!available_options[i] && i != removed_ele)
				increment += (3 * i);
		}
	int initial_num_lowers = initial_num_options - initial_num_uppers;

	if (removed_ele <= sixes)
		reduced_initial_num_uppers --;
	else
		initial_num_lowers --;
	//if too many uppers would need to be filled in to obtain equilibrium, then the path is to be disfavored
	//we must take care to exclude removed element


	if ((initial_num_lowers - curr_num_lowers) + 1 < (reduced_initial_num_uppers - curr_num_uppers))
	{
		return max(0, increment - 1);
	}
	//why is it so disfavorable to choose this? Perhaps this shouldn't have the effect of discouraging CPU to go for bonus, just to
	//shift paths! But how?


	//calculate the equilibrium score
	int equilibrium = 63;
	for (int i=1; i<=6; i++)
		if (available_options[i])

			equilibrium -= 3 * i;

	if (initial_upper_score + increment < equilibrium && initial_num_uppers >= 3 && initial_num_options >= 5)
		for (int i=1; i<=6; i++)
			if (initial_options[i])
			{
				int second_lowest_rank = i + 1;
				for (; second_lowest_rank <= sixes; second_lowest_rank ++)
					if (initial_options[second_lowest_rank])
						break;

				//increment += min(equilibrium - initial_upper_score, i + (second_lowest_rank - i) * (initial_num_options - 5) / 7);

				increment += min(equilibrium - initial_upper_score, i * (second_lowest_rank / i));
				//willing to make up one score. Because of the integer division,
				//not simply second_lowest_rank
				break;
			}

	return increment;
}


typedef struct
{
	part_three_predicted_scores data;
	double stored_probability;
	int old_removed_ele;
} expanded_part_three_predicted_scores;


typedef storing_considered_rolls (*distribution_calculator)(bool [], int, part_three_predicted_scores [], \
		storing_considered_rolls_with_overall_value [], storing_considered_rolls_with_overall_value [], int);

//This class computes the adjustments for a fixed set of options.
class Beynesian_Computation
{
	//initial data
	int initial_num_uppers;
	bool initial_options[14];
	int original_base_index;
	int n;
	distribution_calculator get_distribution;
	storing_considered_rolls_with_overall_value *decision_input = \
			new storing_considered_rolls_with_overall_value[9331];
	storing_considered_rolls_with_overall_value *decision_output = \
			new storing_considered_rolls_with_overall_value[252];

	//data which dynamically changes
	bool *available_options; int curr_base_index; int indices_after_plucking[14];

	int path_len;
	int len;
	int *subset_correspondence = NULL;
	int *sorted_by_num_ones;
	vector<vector<int>> subsets;
	vector<vector<double>> intermediate_probs;
	int *elements;
	int partitions_in_num_ones[13];
	int initial_upper_score;
	double *probs_upper_value [8192];

public:

	/* Inputs:
	 * get_distribution: pointer to function which calculates distribution
	 * available_options: options not yet crossed off on scoresheet
	 * initial_upper_score: the number of points player has from upper categories only
	 * Putting any value >=63 is equivalent. Undefined behavior if is negative
	 */
	Beynesian_Computation(distribution_calculator get_distribution, \
			bool *available_options, int initial_upper_score)
	{
		this -> get_distribution = get_distribution;
		this -> available_options = available_options;
		this -> initial_upper_score = initial_upper_score;

		memcpy(initial_options + 1, available_options + 1,  13 * sizeof(bool));


		original_base_index = convert_base_2_to_10(13, initial_options + 1);


		initial_num_uppers = 0;
		for (int i=1; i<=6; i++)
			if (initial_options[i])
				initial_num_uppers ++;
		path_len = -1; //since something will be plucked.
		for (int i=1; i<=13; i++)
			if (available_options[i])
				path_len ++;
		len = pow(2,path_len);
		sorted_by_num_ones = init_sorted_by_num_ones();
		for (int i=0; i < 8192; i++)
		{
			vector<double> v(13, (i == 0));
			intermediate_probs.push_back(v);
		}

		elements = new int [path_len];
		for (int removed_ele = ones; removed_ele <= chance; removed_ele ++)
			if (initial_options[removed_ele])
			{
				for (int i=0; i<8192; i++)
				{
					part_three_arr[i][removed_ele - 1].score_independent_from_freq = 0;
					for (int l=0; l<4; l++)
						part_three_arr[i][removed_ele - 1].freq_dependent_scores[l] = 0;
				}
			}

		int index_in_partitions = 0;
		for (int i=0; i<len-1; i++)
			if (num_ones(sorted_by_num_ones[i]) < num_ones(sorted_by_num_ones[i+1]))
			{
				partitions_in_num_ones[index_in_partitions] = i;
				index_in_partitions ++;
			}
		partitions_in_num_ones[index_in_partitions] = len;

		for (int i=0; i<len; i++)
		{
			vector<int> curr_subset;
			for (int j=0; j<32; j++)
				if (i & (1 << j))
					curr_subset.push_back(j);
			subsets.push_back(curr_subset);
		}

		for (int i=0; i<8192; i++)
		{
			probs_upper_value[i] = new double [64];
			for (int j=0; j<64; j++)
				probs_upper_value[i][j] = (i == 0 && (j == initial_upper_score || (j == 63 && initial_upper_score > 63)));
		}


	}

	void constructor_given_removed_ele(int removed_ele)
	{
		memcpy(available_options + 1, initial_options + 1, 13 * sizeof(bool));
		available_options[removed_ele] = false;
		curr_base_index = convert_base_2_to_10(13, available_options + 1);

		int index_in_elements = 0;
		vector<int> vec_form_elements;

		for (int i=1; i<=13; i++)
			if (available_options[i])
			{
				vec_form_elements.push_back(i);
				elements[index_in_elements] = i;
				index_in_elements ++;
			}
		delete [] subset_correspondence;
		subset_correspondence = index_correspondence_subsets(vec_form_elements, path_len, true);

	}


	~Beynesian_Computation()
	{
		free(subset_correspondence);
		free(sorted_by_num_ones);
		free(elements);
		for (int i=0; i<8192; i++)
			delete probs_upper_value[i];
		delete [] decision_input;
		delete [] decision_output;
	}

	//Utility function for obtaining the appropriate element of part_three_arr outside of this class
	int get_curr_base_index()
	{
		return curr_base_index;
	}

	//sorts the indices from 0 to 2^path_len using "compare_num_ones" as the comparison function
	int *init_sorted_by_num_ones()
	{
		int *result = new int [len];
		for (int i=0; i<len; i++)
			result[i] = i;
		qsort(result, len, sizeof(int), compare_num_ones);
		//sorted first by num_ones, and then by numerical order
		return result;
	}

	//adds the value of the category to "result." In some cases, uses the sums stored in a row from find_redundency
	void apply_category_value(int category, int row_in_find_redundency, double &result)
	{
		switch (category)
		{
		case three_of_a_kind:
			result += find_redundency[row_in_find_redundency]-> sums[1];
			break;
		case four_of_a_kind:
			result += find_redundency[row_in_find_redundency] -> sums[2];
			break;
		case full_house: case small_strait: case large_strait: case yahtzee:
			result += global_lower_scores[category];
			break;
		case chance:
			result += find_redundency[row_in_find_redundency] -> sums[0];
			break;
		}
	}

	void init_lower_values_per_index(double lower_values_per_index [], int lower_lim, int upper_lim)
	{

		for (int index = lower_lim; index < upper_lim; index ++)
		{
			int i = subset_correspondence[sorted_by_num_ones[index]];
			lower_values_per_index[i] = 0;
			for (int category = ones; category <= chance; category ++)
			{
				if (i & (1 << (13 - category)))
				{
					apply_category_value(category, i, lower_values_per_index[i]);
				}

			}
		}
	}


	void init_heuristic_base_upper_scores(double heuristic_base_upper_scores [], int lower_lim, int upper_lim, int removed_ele)
	{
		int initial_num_options = 0;
		for (int i=1; i<14; i++)
			if (initial_options[i])
				initial_num_options ++;
		for (int index = lower_lim; index < upper_lim; index ++)
		{
			int i = subset_correspondence[sorted_by_num_ones[index]];
			vector<bool> vec_form_options = convert_key_to_subset(i);
			for (int j=1; j<14; j++)
				available_options[j] = vec_form_options[j-1];
			int curr_num_uppers = 0, curr_num_lowers = 0;
			for (int j=1; j<14; j++)
				if (available_options[j])
				{
					if (j <= sixes)
						curr_num_uppers ++;
					else
						curr_num_lowers ++;
				}

			heuristic_base_upper_scores[i] = (curr_num_uppers) ? \
					compute_heuristic_increment(initial_options, available_options, initial_upper_score, \
							initial_num_options, initial_num_uppers, removed_ele, curr_num_uppers, curr_num_lowers) : 0;
		}

	}

	int sum_entry(int *node_subset)
	{
		int score = 0;
		for (int i=1; i<=6; i++)
		{
			if (node_subset[i] != -1)
				score += node_subset[i];
		}
		return score;
	}

	//adds the contribution of the bonus to the part three array, and stores the old value in score_independent_from_bonus
	void process_bonus_event(int i, int removed_ele, double heuristic_base_upper_scores[], \
			double results_indep_bonus[8192][13])
	{

		double base_score = heuristic_base_upper_scores[subset_correspondence[i]];

		results_indep_bonus[subset_correspondence[i]][removed_ele - 1] = \
				part_three_arr[subset_correspondence[i]][removed_ele - 1].score_independent_from_freq;

		normalize_arr(64, probs_upper_value[subset_correspondence[i]]);


		for (int j=initial_upper_score; j<=63; j++)
		{
			if (j + base_score >= 63)
				part_three_arr[subset_correspondence[i]][removed_ele - 1].score_independent_from_freq \
				+= 35 * probs_upper_value[subset_correspondence[i]][j];
			else if (removed_ele <= sixes)
			{
				for (int k=1; k<=4; k++)
					if (j + base_score + k * removed_ele >= 63)
					{
						part_three_arr[subset_correspondence[i]][removed_ele - 1].freq_dependent_scores[k-1] \
						+= 35 * probs_upper_value[subset_correspondence[i]][j];
					}
			}


		}

		if (initial_upper_score > 63)
			part_three_arr[subset_correspondence[i]][removed_ele - 1].score_independent_from_freq += 35;
	}

	//Updates the probability and expected value of a set based on data at a single node and the data stored for
	//the set minus the node. The expected upper values are updated using convolution.
	void process_event(int i, int j, double value, int category_index, int element_index,  int removed_ele, \
			double results_indep_bonus[][13], int freq = 0)
	{
		double probability_to_add = intermediate_probs[i - powers_of_two_array[subsets[i][j]]][removed_ele - 1] \
				* find_redundency[subset_correspondence[i]] -> Probabilities[category_index];

		if (freq == 0)
			part_three_arr[subset_correspondence[i]][removed_ele - 1].score_independent_from_freq += probability_to_add * \
			(value + results_indep_bonus[subset_correspondence[i] - powers_of_two_array[13 - element_index]][removed_ele - 1]);

		else
			part_three_arr[subset_correspondence[i]][removed_ele - 1].freq_dependent_scores[freq] += probability_to_add * \
				(35 + results_indep_bonus[subset_correspondence[i] - powers_of_two_array[13 - element_index]][removed_ele - 1]);

		intermediate_probs[i][removed_ele - 1] += probability_to_add;

		for (int k=initial_upper_score; k<=63; k++)
		{
			int column = fmin(k + ((element_index <= sixes) ? value : 0), 63);

			probs_upper_value[subset_correspondence[i]][column] += probability_to_add * \
			probs_upper_value[subset_correspondence[i] - powers_of_two_array[13 - element_index]][k];

		}

		if (initial_upper_score > 63)
			probs_upper_value[subset_correspondence[i]][63] += probability_to_add;

	}

	/*
	Uses dynamic programming to efficiently calculate adjustments to expected values based on the consequences
	of a specific selection to the rest of the game. We are virtually considering each permutation of options.
	But we avoid this overhead by considering the value of a particular situation in terms of the situation
	where exactly one option is plucked. We don't know which node is plucked or where, so we consider each
	possibility. To be more explicit: for each number of options n <= path_len, calculate all distributions
	for sets of options having order n. Break up the task based on the option which we consider for the current
	turn. For each subset S of order n, enumerate over the n subsets of S with order n-1, and update the probability
	and expected value of S as explained above.
	 */
	void compute_beynesian_expected_values()
	{
		int index_in_partitions = 0;
		double lower_values_per_index[8192];
		double heuristic_base_upper_scores[8192];
		indices_after_plucking[0] = initial_upper_score;
		int category_allowed_take_zero[8192];
		double results_indep_bonus[8192][13] = {0};

		expanded_part_three_predicted_scores (*array_visited_results)[8192] = (expanded_part_three_predicted_scores ((*) [8192]))\
				(malloc(8192 * sizeof(expanded_part_three_predicted_scores)));
		for (int j=0; j<8192; j++)
				(*array_visited_results)[j].data.score_independent_from_freq = -1; //marks off that nothing stored yet

		while (partitions_in_num_ones[index_in_partitions] < len)
		{
			int curr_num_ones = index_in_partitions + 1;
			int lower_lim = partitions_in_num_ones[index_in_partitions] + 1;
			int upper_lim = fmin(partitions_in_num_ones[index_in_partitions + 1] + 1, len);

			for (int removed_ele = ones; removed_ele <= chance; removed_ele ++)
			{
				if (! initial_options[removed_ele])
					continue;


				constructor_given_removed_ele(removed_ele);

				//before starting, update all the distributions. This is important in case of adjustments

				//add one to index in sorted by num ones because partitions defined s.t. last element of curr # zeroes

				//before starting, update all the distributions. This is important in case any of the adjustments change

				for (int index = lower_lim; index < upper_lim; index ++)
				{
					int i = sorted_by_num_ones[index];

					assert(num_ones(i) == curr_num_ones);
					vector<bool> vec_form_available_options = convert_key_to_subset(subset_correspondence[i]);
					for (int j=1; j<14; j++)
						available_options[j] = vec_form_available_options[j-1];
					curr_base_index = subset_correspondence[i];

					if (find_redundency[curr_base_index] == NULL)
					{
						find_redundency[curr_base_index] = \
								(storing_considered_rolls *)(malloc(sizeof(storing_considered_rolls)));

						part_three_predicted_scores adjustments [13];

						//construct the leaves from the stored root
						for (int i=1; i<14; i++)
						{
							if (! available_options[i])
								continue;
							indices_after_plucking[i] = curr_base_index - powers_of_two_array[13 - i];
							adjustments[i-1] = part_three_arr[indices_after_plucking[i]][i-1];
						}

						*(find_redundency[curr_base_index]) = \
								get_distribution (available_options, initial_upper_score, \
								adjustments, decision_input, decision_output, 3);

						for (int i=0; i<12; i++)
							if (find_redundency[curr_base_index] -> Probabilities [31 + i])
							{
								find_redundency[curr_base_index] -> catgr_may_take_zero = i + 1;
								category_allowed_take_zero[curr_base_index] = i + 1;
								break;
							}
					}
				}



				//now, calculate the scores corresponding to indices and node subsets
				init_lower_values_per_index(lower_values_per_index, lower_lim, upper_lim);
				init_heuristic_base_upper_scores(heuristic_base_upper_scores, lower_lim, upper_lim, removed_ele);


				for (int index = lower_lim; index < upper_lim; index ++)
				{
					int i = sorted_by_num_ones[index];

					curr_base_index = subset_correspondence[i];

					if (removed_ele > sixes && (*array_visited_results)[curr_base_index].data.score_independent_from_freq != -1)
					{

						//are you sure that supposed to be doing this at row curr_base_index?
						intermediate_probs[i][removed_ele - 1] = (*array_visited_results)[curr_base_index].stored_probability;
						part_three_arr[curr_base_index][removed_ele - 1] = (*array_visited_results)[curr_base_index].data;

						results_indep_bonus[curr_base_index][removed_ele - 1] = \
								results_indep_bonus[curr_base_index][(*array_visited_results)[curr_base_index].old_removed_ele - 1];
						continue;
					}


					int size = subsets[i].size();
					for (int j=0; j<size; j++)
					{
						int element_index = elements[subsets[i][j]];
						//first, consider taking a zero

						if (element_index == category_allowed_take_zero[subset_correspondence[i]])
						{

							int taking_zero_category = element_index + 30;

							process_event(i, j, 0, taking_zero_category, element_index, removed_ele, results_indep_bonus);
						}

						//next, consider taking a score
						if (element_index <= sixes)
						{
							for (int k=1; k<=4; k++)
								process_event(i, j, k * element_index, 4 * (element_index - 1) + (k-1), element_index, removed_ele, \
										results_indep_bonus);

						}
						else
						{
							int taking_score_category = element_index + 17;
							double value = 0;
							apply_category_value(element_index, subset_correspondence[i], value);
							process_event(i, j, value, taking_score_category, element_index, removed_ele, \
									results_indep_bonus);

						}
					}

					process_bonus_event(i, removed_ele, heuristic_base_upper_scores, results_indep_bonus);


					if (removed_ele > sixes && partitions_in_num_ones[curr_num_ones] < len)
					{

						(*array_visited_results)[curr_base_index].data = part_three_arr[curr_base_index][removed_ele - 1];
						(*array_visited_results)[curr_base_index].stored_probability = \
								intermediate_probs[i][removed_ele - 1];
						(*array_visited_results)[curr_base_index].old_removed_ele = removed_ele;
					}

				}

			}

			index_in_partitions ++;
		}

		curr_base_index = original_base_index;
		for (int l = 1; l<14; l++)
			indices_after_plucking[l] = curr_base_index - powers_of_two_array[13 - l];

		if (initial_options[ones] && path_len >= 5)
			part_three_arr[indices_after_plucking[1]][0].score_independent_from_freq -= 5;


		for (int removed_ele = ones; removed_ele <= chance; removed_ele ++)
			if (initial_options[removed_ele])
				part_three_arr[original_base_index][removed_ele - 1] = \
				part_three_arr[indices_after_plucking[removed_ele]][removed_ele - 1];

		free(array_visited_results);
		memcpy(available_options + 1, initial_options + 1, 13 * sizeof(bool));


	}
};

//corrects the indices to account for noncontiguous indices.
int *index_correspondence_subsets(vector<int> &elements, int path_len, bool is_reversing)
{
	int num_subsets = pow(2,path_len);
	int *result = (int *)(malloc(num_subsets * sizeof(int)));
	for (int i=0; i<num_subsets; i++)
	{
		vector<int> old_indices;
		for (int j=0; j<path_len; j++)
			if (i & (1 << j))
				old_indices.push_back(j);

		if (is_reversing)
		{
			int size = old_indices.size();
			bool boolean_form[13] = {false};
			for (int j=0; j<size; j++)
			{
				boolean_form[elements[old_indices[j]] - 1] = true;
			}
			result[i] = convert_base_2_to_10(13, boolean_form);
		}
		else
		{
			result[i] = 0;
			for (unsigned int j=0; j<old_indices.size(); j++)
				result[i] += powers_of_two_array[elements[old_indices[j]]];
		}
	}

	return result;
}

void print_ele_part_three_arr(int i, int j)
{
	cout << part_three_arr[i][j].score_independent_from_freq << " ";
	for (int k = 0; k < 4; k++)
		cout << part_three_arr[i][j].freq_dependent_scores[k] << " ";
	cout << endl;
}

//Clears the adjustments array and the distributions
void clear_out_part_three_arr()
{
	for (int i=0; i<8192; i++)
	{
		free(find_redundency[i]);
		find_redundency[i] = NULL;
		for (int j=0; j<13; j++)
		{
			part_three_arr[i][j].score_independent_from_freq = 0;
			for (int k=0; k<4; k++)
				part_three_arr[i][j].freq_dependent_scores[k] = 0;
		}
	}
}
