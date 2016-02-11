#include "Auxillary.cpp"
#include <algorithm>
#include <string.h>

//Returns whether a given roll is an instance of one of the categories (for example, "55556" is not a Yahtzee but is 4-k
bool CategoryInfo::is_valid_option(int category, int rolls[5], bool available_options[NUM_CATEGORIES+1])
{
	if (available_options[category] == false) return false;
	switch(category)
	{
	case three_of_a_kind: case four_of_a_kind: case yahtzee: case full_house:
	{
		int found_element = 0;
		int frequencies[6] = {0};
		int required_frequency = (category == three_of_a_kind || \
				category == full_house) ? 3 : \
						((category == four_of_a_kind) ? 4 : 5);
		for (int i=0; i<5; i++)
		{
			frequencies[rolls[i] - 1] ++;
			if ((frequencies[rolls[i] - 1]) == required_frequency)
			{
				if (category == full_house)
				{

					if (rolls[i] == found_element) continue;
					if (required_frequency == 2) return true;
					found_element = rolls[i];
					required_frequency = 2;
					i = -1;
					int zeros[6] = {0};
					memcpy(frequencies, zeros, 6*sizeof(int));
				}
				else return true;
			}
		}
		return false;
	}
	case small_strait: case large_strait:
	{
		int copy_of_rolls[5];
		memcpy(copy_of_rolls, rolls, 5*sizeof(int));
		sort(copy_of_rolls, copy_of_rolls + 5);
		int required_strait_length = (category == small_strait ? 4 : 5);
		int strait_length_so_far = 1;
		for (int i = 1; i < 5; i++)
		{
			if (copy_of_rolls[i] == copy_of_rolls[i-1] + 1)
			{
				strait_length_so_far ++;
				if (strait_length_so_far == required_strait_length) return true;
			}
			else if (copy_of_rolls[i] > copy_of_rolls[i-1]) strait_length_so_far = 1;
		}
		return false;
	}
	default: return true;
	}
}

//Given a specific roll, adjustments, options, whether is last round, and upper score, returns
//a structure containing the decision and the expected value of the decision.
value_of_roll CategoryInfo::make_choice(int roll [5], part_three_predicted_scores weights_part_3[13], \
		bool available_options[14], bool is_last_round, int curr_upper_score)
{
	int choice[3] = {0};
	double contender;
	int lower_scores[7] = {sum_array(5, roll), sum_array(5, roll), 25, 30, 40, 50, sum_array(5, roll)};
	//upper categories

	double alternates[3] = {0, 0, -1};
	//if we cannot take a zero in any category we have allowed, we must avoid returning choice[2]

	for (int i = 1; i<three_of_a_kind; i++)
	{

		if (!available_options[i]) continue;

		int freq = freq_ele_in_arr(i, 5, roll);

		contender =  weights_part_3[i-1].score_independent_from_freq;

		if (freq)
		{
			contender += freq * i; //start with score for current turn. Then add on contribution
			if (is_last_round && contender + curr_upper_score >= 63)
				contender += 35;
			//from the rest of the game
			if (freq < 5)
				contender += weights_part_3[i-1].freq_dependent_scores[freq - 1];
			else
				contender += weights_part_3[i-1].freq_dependent_scores[3];
		}


		//depending on the frequency, add the corresponding elements part_three arr adjustments
		//if freq is zero, add nothing. If freq is five, add the numbers as if freq were four.


		if (alternates[0] < contender)
		{
			alternates[0] = contender;
			choice[0] = i;
		}


		//the problem: your row denotes the result of plucking out one of the available options

	}
	//lower categories
	for (int i=three_of_a_kind; i<=chance; i++)
	{
		if (i < chance && !is_valid_option(i, roll, available_options)) continue;
		if (!available_options[i]) continue;
		contender = lower_scores[i-three_of_a_kind] + weights_part_3[i-1].score_independent_from_freq;


		if (is_last_round && curr_upper_score >= 63)
			contender += 35;

		//if better option, or a couple of special cases
		if (alternates[1] < contender)
		{
			alternates[1] = contender;
			choice[1] = i;
		}


	}

	//now check the possibility of taking a zero in a category
	//if is upper category, subject the result to the bonus effect later on!!!
	for (int i = 0; i < 12; i++)
	{
		if (!available_options[i + 1]) continue;
		if (alternates[2] < weights_part_3[i].score_independent_from_freq)
		{

			alternates[2] = weights_part_3[i].score_independent_from_freq;
			choice[2] = 31 + i;
		}
	}

	double best_alternate = max_array(3, alternates);

	value_of_roll result;

	if (best_alternate == alternates[2])
	{
		result.option = choice[2];
		result.value += alternates[2];
	}
	else if (best_alternate == alternates[0])
	{
		result.option = choice[0];
		result.value += alternates[0];
	}

	else
	{
		result.option = choice[1];
		result.value += alternates[1];
	}
	return result;


}

