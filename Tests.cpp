#include "YahtzeeDistributions.cpp"

void test_make_choice()
{
	int weights[252][32] = {0};
	find_num_ways_get_superset_from_subset(weights);
	bool available_options[14];
	for (int i=1; i<14; i++) available_options[i] = (i == sixes || i==chance || i==yahtzee);
	int roll[5] = {1,2,5,6,6};

	//part 3 adjustment example
	part_three_predicted_scores weights_part_3[13];
	weights_part_3[sixes - 1].score_independent_from_freq = 12;
	for (int i=2; i<4; i++)
		weights_part_3[sixes - 1].freq_dependent_scores[i] = 35;
	weights_part_3[yahtzee - 1].score_independent_from_freq = 23 + 10;
	weights_part_3[chance - 1].score_independent_from_freq = 2 + 10;

	CategoryInfo instance = CategoryInfo();
	value_of_roll result = instance.make_choice(roll, weights_part_3, available_options, false, 0);

	assert(result.option == 42); //zero in yahtzee
	assert(fabs(result.value - 33) <= .001);

	//now, we change 5 into six, and expect to take sixes
	roll[2] = 6;
	result = instance.make_choice(roll, weights_part_3, available_options, false, 0);
	assert(result.option == sixes);

}

void test_get_distribution()
{
	bool available_options[14];
	for (int i=1; i<14; i++) available_options[i] = (i == sixes || i==chance || i==yahtzee);
	part_three_predicted_scores weights_part_3[13];
	weights_part_3[sixes - 1].score_independent_from_freq = 12;
	for (int i=2; i<4; i++)
		weights_part_3[sixes - 1].freq_dependent_scores[i] = 35;
	weights_part_3[yahtzee - 1].score_independent_from_freq = 23 + 10;
	weights_part_3[chance - 1].score_independent_from_freq = 2 + 10;


	storing_considered_rolls_with_overall_value *decision_input = \
			new storing_considered_rolls_with_overall_value[9331];
	storing_considered_rolls_with_overall_value *decision_output = \
			new storing_considered_rolls_with_overall_value[252];


	storing_considered_rolls result = get_distribution_or_choice(available_options, 45, weights_part_3, decision_input, \
			decision_output);

	for (int i=0; i<NUM_OPTIONS; i++)
	{
		if (i == 24 || i == 31)
			cout << "| ";
		cout << result.Probabilities[i] << " ";
	}
	cout << endl;
	for (int i=0; i<3; i++)
		cout << result.sums[i] << " ";
	cout << endl;



	delete [] decision_input;
	delete [] decision_output;
}

void test_get_distribution2()
{
	//these are the actual adjustments from part 3 for distribution -4-k,-FH
	double adjustments[13][5] =
	{
			{181.739,	1.69964,	3.40115,	5.14604,	6.82755},
			{180.215,	3.13052,	6.56007,	10.1377,	13.3759},
			{173.51,	4.00768,	9.08712,	14.541, 	19.4759},
			{167.936,	3.61435,	9.8799, 	17.5729,	24.1774},
			{164.181,	2.05778,	8.58621,	18.5807,	27.1642},
			{161.888,	0.594032,	5.87712,	18.2263,	29.0429},
			{172.017,	0,	0,	0,	0},
			{0,	0,	0,	0,	0},
			{0,	0,	0,	0,	0},
			{161.351,	0,	0,	0,	0},
			{166.521,	0,	0,	0,	0},
			{180.489,	0,	0,	0,	0},
			{165.929,	0,	0,	0,	0}
	};

	part_three_predicted_scores weights_part_3[13];
	for (int i=0; i<13; i++)
	{
		weights_part_3[i].score_independent_from_freq = adjustments[i][0];
		for (int j=0; j<4; j++)
			weights_part_3[i].freq_dependent_scores[j] = adjustments[i][j+1];
	}
	bool available_options[14];
	for (int i=1; i<14; i++) available_options[i] = (i != four_of_a_kind && i != full_house);

	storing_considered_rolls_with_overall_value *decision_input = \
			new storing_considered_rolls_with_overall_value[9331];
	storing_considered_rolls_with_overall_value *decision_output = \
			new storing_considered_rolls_with_overall_value[252];


	storing_considered_rolls result = get_distribution_or_choice(available_options, 0, weights_part_3, decision_input, \
			decision_output);

	for (int i=0; i<NUM_OPTIONS; i++)
	{
		if (i == 24 || i == 31)
			cout << "| ";
		cout << result.Probabilities[i] << " ";
	}
	cout << endl;
	for (int i=0; i<3; i++)
		cout << result.sums[i] << " ";
	cout << endl;



	delete [] decision_input;
	delete [] decision_output;

}



void test_adjustments()
{
	bool available_options[14];
	for (int i=1; i<14; i++) available_options[i] = (i != four_of_a_kind && i != full_house);
	int num_rounds_left = 0;
	for (int i=1; i<14; i++)
		if (available_options[i])
			num_rounds_left ++;
	int initial_upper_score = 0;
	for (int i=1; i<=6; i++)
		if (!available_options[i])
			initial_upper_score += 3 * i;
	if (num_rounds_left > 1)
	{
		Beynesian_Computation computer = Beynesian_Computation(&get_distribution_or_choice, \
				available_options, initial_upper_score);
		computer.compute_beynesian_expected_values();
		for (int i=0; i<13; i++)
			print_ele_part_three_arr(computer.get_curr_base_index(), i);
		for (int i=0; i<8192; i++)
		{
			free(find_redundency[i]);
			find_redundency[i] = NULL;
		}
	}

}

void test_intermed_choice()
{
	//uses actual adjustments

	bool available_options[14];
	for (int i=1; i<14; i++) available_options[i] = (i == fives || i==chance || i==yahtzee);
	part_three_predicted_scores weights_part_3[13];
	weights_part_3[fives - 1].score_independent_from_freq = 27.2598;
	for (int i=2; i<4; i++)
		weights_part_3[fives - 1].freq_dependent_scores[i] = 35;
	weights_part_3[yahtzee - 1].score_independent_from_freq = 53.9569;
	weights_part_3[chance - 1].score_independent_from_freq = 34.3179;


	storing_considered_rolls_with_overall_value *decision_input = \
			new storing_considered_rolls_with_overall_value[9331];
	storing_considered_rolls_with_overall_value *decision_output = \
			new storing_considered_rolls_with_overall_value[252];

	vector<vector<int>> supersets = get_supersets();

	get_distribution_or_choice(available_options, 48, weights_part_3, decision_input, \
			decision_output, 1);

	vector<int> sample_rolls = {2,5,5,6,6};
	vector<int> expected_choice = {5,5};


	int superset_index = find(supersets.begin(), supersets.end(), sample_rolls) - supersets.begin();
	int subset_index = get_index(expected_choice);


	//if the choice is 5,5, as expecting,
	// then expect that decision_output[superset index].value equals decision_input[subset_index].value

	assert(abs(decision_input[subset_index].value - decision_output[superset_index].value) <= .001);


	vector<int> sample_rolls_2 = {1,1,1,2,2};
	int superset_index_2 = find(supersets.begin(), supersets.end(), sample_rolls_2) - supersets.begin();
	vector<int> expected_choice_2 = vector<int> (0);
	subset_index = get_index(expected_choice_2);
	assert(abs(decision_input[subset_index].value - decision_output[superset_index_2].value) <= .001);

	get_distribution_or_choice(available_options, 48, weights_part_3, decision_input, \
			decision_output, 2);

	vector<int> expected_choice_3 (3,1);
	subset_index = get_index(expected_choice_3);
	assert(abs(decision_input[subset_index].value - decision_output[superset_index_2].value) <= .001);

	for (unsigned int i=0; i<Memoized_Subsets.subset_array[superset_index_2].size(); i++)
	{
		int subset_index = Memoized_Subsets.indices[superset_index_2][i];
		if(abs(decision_input[subset_index].value - decision_output[superset_index_2].value) <= .001)
		{
			vector<int> claimed_ele = Memoized_Subsets.subset_array[superset_index_2][i];
			assert(claimed_ele == expected_choice_3);
			break;
		}
		assert(i < Memoized_Subsets.subset_array[superset_index_2].size() - 1);

	}

	delete [] decision_input;
	delete [] decision_output;
}
