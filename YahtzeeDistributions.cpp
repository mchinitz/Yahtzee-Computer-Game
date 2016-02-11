#include "CategoryInfo.cpp"
#include "WeightCalculations.cpp"
#include "Compute_Probs.cpp"

//TODO Organize all this stuff


//Returns the index of a given subset within the subset array
int get_index(vector<int> &subset)
{
	int subset_arr[5] = {0};
	if (subset.size())
		memcpy(subset_arr + (5 - subset.size()), &(subset[0]), subset.size() * sizeof(int));
	return find_powered_key(5, subset_arr);
}

//Given a set of options, the last-roll choices, and all supersets, returns the set of options that
//are represented by at least one roll
vector<int> get_categories_to_update(bool available_options[14], value_of_roll last_round_input [252],\
		vector<vector<int>> &supersets)
{
	bool is_represented[NUM_OPTIONS] = {false};
	for (int i=0; i<252; i++)
	{
		if (last_round_input[i].option <= sixes)
			is_represented[(last_round_input[i].option - 1) * 4 + \
			 min(4, freq_ele_in_arr(last_round_input[i].option, 5, &(supersets[i][0]))) - 1] = true;
		else if (last_round_input[i].option <= chance)
			is_represented[last_round_input[i].option + 17] = true;
		else
			is_represented[last_round_input[i].option] = true;
	}

	vector<int> categories_to_update;
	for (int i=ones; i<=chance; i++)
	{
		if (!(available_options[i]))
			continue;
		if (i <= sixes)
		{
			for (int j=(i-1) * 4; j < i * 4; j++)
				if (is_represented[j])
					categories_to_update.push_back(j);
		}
		else
		{
			if (is_represented[i + 17])
				categories_to_update.push_back(i + 17);
		}
		if (i != chance && is_represented[i + 30])
			categories_to_update.push_back(i + 30);
	}


	return categories_to_update;
}

//Memoizes the subsets of each roll, and the indices for them, for otherwise recreating them over and over is
//too expensive.

struct Memoized_Subsets_Fields
{
	vector<vector<vector<int>>> subset_array;
	vector<vector<int>> indices;
	vector<int> all_indices;
};


static class memoized_Subsets
{
public:

	const Memoized_Subsets_Fields init_fields = get_fields();

	const vector<vector<vector<int>>> &subset_array = init_fields.subset_array;
	const vector<vector<int>> & indices = init_fields.indices;
	const vector<int> & all_indices = init_fields.all_indices;

	Memoized_Subsets_Fields get_fields()
	{
		Memoized_Subsets_Fields result =  Memoized_Subsets_Fields();
		vector<vector<int>> supersets = get_supersets();
		for (vector<int> superset : supersets)
		{
			result.subset_array.push_back(get_subsets((&superset[0])));
			result.indices.push_back(vector<int> ());
			for (vector<int> subset : result.subset_array[result.subset_array.size() - 1])
			{
				int index = get_index(subset);
				result.indices[result.indices.size() - 1].push_back(index);
				if (find(result.all_indices.begin(), result.all_indices.end(), index) == result.all_indices.end())
					result.all_indices.push_back(index);

			}

		}
		return result;

	}
} Memoized_Subsets;

//Copies the value, Probabilities, and sums stored in "in" into "out". Simply using "=" leads to overlap
void deep_cpy(storing_considered_rolls_with_overall_value &in, storing_considered_rolls_with_overall_value &out)
{
	out.value = in.value;
	for (int i=0; i<NUM_OPTIONS; i++)
		out.info.Probabilities[i] = in.info.Probabilities[i];
	for (int i=0; i<3; i++)
		out.info.sums[i] = in.info.sums[i];
}

//For each possible finishing roll on the third turn of the round, calls the function "make_choice"
//and stores the result
void last_round_decisions(bool available_options[NUM_CATEGORIES + 1], int curr_upper_score, \
		part_three_predicted_scores weights_part_three[13], value_of_roll decision_input[252],\
		vector<vector<int>> &supersets)
{

	bool is_last_round = (freq_ele_in_arr(true, 13, available_options + 1) == 1);
	static CategoryInfo instance = CategoryInfo();

	for (int i=0; i<252; i++)
		decision_input[i] = \
		instance.make_choice(&(supersets[i][0]), weights_part_three, available_options, \
				is_last_round, curr_upper_score);

}

//The results of a prior decision are combined, in the sense that the probability of an event is
//the sum of the probabilities of each outcome in the event. Decision_output is the key input, and decision input
//stores the results.
void sum_outcomes_on_path(storing_considered_rolls_with_overall_value decision_output[252], \
		storing_considered_rolls_with_overall_value decision_input[9331], int weights[252][32], \
		vector<vector<int>> &supersets, vector<int> &categories_to_update, bool only_consider_empty_subset = false)
{

	for (int i=0; i<462; i++)
		decision_input[Memoized_Subsets.all_indices[i]] = storing_considered_rolls_with_overall_value(); //clears out old data

	//vector<int> categories_to_update = get_categories_to_update(available_options);
	int *arr_form_catgr_to_update = &(categories_to_update[0]);

	for (int i=0; i<252; i++)
	{
		const vector<vector<int>> &subsets = Memoized_Subsets.subset_array[i];
		int num_subsets = (only_consider_empty_subset) ? 1 : subsets.size();


		for (int j=0; j<num_subsets; j++)
		{
			int index = Memoized_Subsets.indices[i][j];
			double quotient = 1.0 * weights[i][j] / powers_of_six_array[5 - subsets[j].size()];
			decision_input[index].value += decision_output[i].value * quotient;

			int size = categories_to_update.size();
			for (int k=0; k<size; k++)
				decision_input[index].info.Probabilities[arr_form_catgr_to_update[k]] += \
				decision_output[i].info.Probabilities[arr_form_catgr_to_update[k]] * quotient;

			for (int k=0; k<3; k++)
				decision_input[index].info.sums[k] += decision_output[i].info.sums[k] * quotient;

		}

	}

	if (only_consider_empty_subset)
	{
		if (decision_input[0].info.Probabilities[three_of_a_kind + 17])
			decision_input[0].info.sums[1] /= decision_input[0].info.Probabilities[three_of_a_kind + 17];
		if (decision_input[0].info.Probabilities[four_of_a_kind + 17])
			decision_input[0].info.sums[2] /= decision_input[0].info.Probabilities[four_of_a_kind + 17];
		if (decision_input[0].info.Probabilities[chance + 17])
			decision_input[0].info.sums[0] /= decision_input[0].info.Probabilities[chance + 17];
	}
}

//Decides which dice to keep, given the resulting expected values of each combination of dice kept
void decide_outcome_on_path(storing_considered_rolls_with_overall_value decision_input[9331], \
		storing_considered_rolls_with_overall_value decision_output[252], \
		vector<vector<int>> &supersets)
{
	for (int i=0; i<252; i++)
	{
		const vector<vector<int>> &subsets = Memoized_Subsets.subset_array[i];
		double max_value_so_far = -1;
		int max_subset_index = -1;
		int num_subsets = subsets.size();
		for (int j=0; j<num_subsets; j++)
		{
			int index = Memoized_Subsets.indices[i][j];
			if (decision_input[index].value > max_value_so_far)
			{
				max_value_so_far = decision_input[index].value;
				max_subset_index = index;
			}
		}
		deep_cpy(decision_input[max_subset_index], decision_output[i]);
	}


}

//Returns the distribution with the specified available options, upper score, and adjustments.
//The last two parameters may be uninitialized.
//The reason for requiring the last two parameters is otherwise, must realloc, or use unique_ptr, which
//results in reduced performance. Not enough room to allocate those arrays on the stack,
//so must go on the heap.
//Note that this function also can be used to make a choice on a turn in the middle of the round. Decision output
//should be used for such a choice, and then the return value is not useful
storing_considered_rolls get_distribution_or_choice(bool available_options[NUM_CATEGORIES + 1], int curr_upper_score, \
		part_three_predicted_scores weights_part_three[13], storing_considered_rolls_with_overall_value decision_input [9331],\
		storing_considered_rolls_with_overall_value decision_output[252], int turns_left)
{

	//initialization
	value_of_roll last_round_input[252];


	static int weights[252][32];
	static vector<vector<int>> supersets;
	static bool is_first_distribution = true;
	if (is_first_distribution)
	{
		find_num_ways_get_superset_from_subset(weights);
		supersets = get_supersets();
		is_first_distribution = false;
	}

	for (int i : Memoized_Subsets.all_indices)
		decision_input[i] = storing_considered_rolls_with_overall_value();
	for (int i=0; i<252; i++)
		decision_output[i] = storing_considered_rolls_with_overall_value();

	//consider last round decisions

	last_round_decisions(available_options, curr_upper_score, weights_part_three, last_round_input, \
			supersets);



	//fill in info into decision_input

	for (int i=0; i<252; i++)
	{
		decision_output[i].value = last_round_input[i].value;
		int option = (last_round_input[i].option <= sixes) ? (last_round_input[i].option - 1) * 4 \
				+ min(freq_ele_in_arr(last_round_input[i].option, 5, &(supersets[i][0])) - 1, 3) : \
				((last_round_input[i].option <= chance) ? last_round_input[i].option + 17 : \
				last_round_input[i].option);

		decision_output[i].info.Probabilities[option] ++;
		if (last_round_input[i].option == three_of_a_kind)
			decision_output[i].info.sums[1] += sum_array(5, &(supersets[i][0]));
		else if (last_round_input[i].option == four_of_a_kind)
			decision_output[i].info.sums[2] += sum_array(5, &(supersets[i][0]));
		else if (last_round_input[i].option == chance)
			decision_output[i].info.sums[0] += sum_array(5, &(supersets[i][0]));
	}


	vector<int> categories_to_update = get_categories_to_update(available_options, last_round_input, supersets);

	for (int i=0; i < min(2, turns_left); i++)
	{
		sum_outcomes_on_path(decision_output, decision_input, weights, supersets, categories_to_update);
		decide_outcome_on_path(decision_input, decision_output, supersets);

	}

	if (turns_left == 3)
		sum_outcomes_on_path(decision_output, decision_input, weights, supersets, categories_to_update, true);

	storing_considered_rolls result = decision_input[0].info;

	return result;
}
