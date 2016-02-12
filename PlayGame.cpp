#include "GetTextfileAdjustments.cpp"
#include <ctype.h>
#include <random>
#include <time.h>
#include <thread>
#include <chrono>
#include <queue>
#include "View.hpp"
#include "Rules.hpp"

extern void Lock();
extern void Unlock();
extern void display_rolling_dice(int, int((*)[2]), bool[14], const char *, int, int[5]);

random_device rd;
mt19937 re(rd());
uniform_int_distribution<int> ui(1, 6);

//the data that must be stored from the computation thread: messages about the CPU's turn, and the round number the messages are attributed to
typedef struct
{
	string message;
	int round_number;

} Message_Queue_Info;


vector<const char *> category_names_without_newlines = { "ones", "twos", "threes", "fours", "fives", "sixes", "three of a kind", \
"four of a kind", "full house", "small strait", "large strait", "yahtzee", "chance" };



//Deals with double-yahtzee scoring, considering that did not incorporate it into the model
class Immediate_Decision : CategoryInfo
{
public:

	bool is_valid_option(int category, int rolls[5], bool available_options[NUM_CATEGORIES+1])
	{
		if (available_options[rolls[0]]) //if did not fill in the upper category that rolled, must play it
			return (category == rolls[0]);
		return available_options[category];
	}

	value_of_roll make_choice_during_game(int roll [5], part_three_predicted_scores weights_part_3[13], \
			bool available_options[14], bool is_last_round, int curr_upper_score)
	{

		CategoryInfo info = CategoryInfo();
		bool initial_status_yahtzee = available_options[yahtzee];
		available_options[yahtzee] = true; //since could have more than one yahtzee
		if (!info.is_valid_option(yahtzee, roll, available_options)) //if the roll is not a yahtzee
		{
			available_options[yahtzee] = initial_status_yahtzee;
			return info.make_choice(roll,weights_part_3,available_options,is_last_round,curr_upper_score);
		}

		available_options[yahtzee] = initial_status_yahtzee;
		if (available_options[yahtzee])
		{
			value_of_roll result;
			result.option = yahtzee;
			return result;
		}

		return this -> make_choice(roll, weights_part_3, available_options, is_last_round, curr_upper_score);
	}
};

//Rules class

bool rules::is_roll_yahtzee(int rolls[5])
{
	return (freq_ele_in_arr(rolls[0], 5, rolls) == 5);

}

//returns whether the category is an available option that is either an upper category or earns a non-zero score
bool rules::is_valid_option(int category, int rolls[5], bool available_options[NUM_CATEGORIES + 1])
{
	if (!is_roll_yahtzee(rolls) || available_options[yahtzee])
	{
		CategoryInfo info = CategoryInfo();
		return info.is_valid_option(category, rolls, available_options);
	}
	Immediate_Decision info = Immediate_Decision();
	return info.is_valid_option(category, rolls, available_options);
}

//given a roll and other information about a player, returns how many points the player would earn by choosing a particular category.
//This is useful not only for updating scores, but also for displaying the scores that the user would get if he or she chose a given category.
int rules::num_pts_for_roll(int choice, int rolls[5], int num_yahtzees, bool available_options[NUM_CATEGORIES + 1])
{
	if (!Rules.is_valid_option(choice, rolls, available_options))
	{
		return 0;
	}
	else if (choice <= sixes)
		return choice * freq_ele_in_arr(choice, 5, rolls);
	else if (global_lower_scores[choice])
		return (int)global_lower_scores[choice] + (((choice == yahtzee) && (num_yahtzees >= 1)) ? 50 : 0);
	else
		return sum_array(5, rolls);
}

//Generic player class. Any player has scores and options to choose from, a method to update the score,
//and a method to choose a category or dice to keep
class Player
{
public:

	Player()
	{
		for (int i=ones; i<=chance; i++)
			available_options[i] = true;
	}


	int getCurrLowerScore() const {
		return curr_lower_score;
	}

	int getCurrUpperScore() const {
		return curr_upper_score;
	}

	int getNumYahtzees() const {
		return num_yahtzees;
	}

	bool isAvailableOption(int i)
	{
		assert(i > 0 && i < 14);
		return available_options[i];
	}

	void update_score(int choice, int rolls[5])
	{
		assert(choice <= chance && choice >= ones);
		assert(available_options[choice]);
		
		int *score_to_update = (choice <= sixes) ? &curr_upper_score : &curr_lower_score;
		(*score_to_update) += Rules.num_pts_for_roll(choice, rolls, num_yahtzees, available_options);

		if (choice != yahtzee || !Rules.is_roll_yahtzee(rolls))
			available_options[choice] = false;
		if (choice == yahtzee && Rules.is_roll_yahtzee(rolls))
			num_yahtzees ++;

	}

	virtual vector<int> make_decision(int turn_number, vector<int> rolls) = 0;

protected:
	int curr_upper_score = 0;
	int curr_lower_score = 0;
	int num_yahtzees = 0;
	bool available_options[NUM_CATEGORIES + 1]; //only applies to the class itself or any type of player.

};

//The computer player makes decisions by maximizing the expected value for the CPU's final score, given the decision.
//Balances the scores earned from the current round with the rest of the game.
class CPU_Player : public Player
{
	Immediate_Decision *info;
	bool used_available_options[14];
	int running_scores[13][2];
public:
	CPU_Player()
	{
		info = new Immediate_Decision();
		for (int i=1; i<14; i++)
			used_available_options[i] = true;
		for (int i = 0; i < 13; i++)
			for (int j = 0; j < 2; j++)
				running_scores[i][j] = -1; //not marked in yet
	}
	~CPU_Player()
	{
		delete info;
	}

	int *get_running_score(int round_number)
	{
		return running_scores[round_number];
	}

	void update_running_scores(int round_number)
	{
		running_scores[round_number][0] = getCurrUpperScore();
		running_scores[round_number][1] = getCurrLowerScore();
	}

	//Returns which dice the computer will keep, given the current dice and allocated arrays (no data must be specified in decision_input or decision_output).
	vector<int> determine_subset_selection(storing_considered_rolls_with_overall_value decision_input[9331], \
			storing_considered_rolls_with_overall_value decision_output[252], vector<int> &rolls)
	{
		vector<vector<int>> supersets = get_supersets();
		int superset_index = find(supersets.begin(), supersets.end(), rolls) - supersets.begin();

		for (unsigned int i=0; i<Memoized_Subsets.subset_array[superset_index].size(); i++)
		{
			int subset_index = Memoized_Subsets.indices[superset_index][i];

			if(abs(decision_input[subset_index].value - decision_output[superset_index].value) <= .001)
				return Memoized_Subsets.subset_array[superset_index][i];
		}
		throw exception();
	}


public:
	//For the first two turns of a round, returns the dice the computer will keep. On the last turn,
	//returns the category chosen (as a 1-element vector)
	vector<int> make_decision(int turn_number, vector<int> rolls)
	{
		int index = convert_base_2_to_10(13, used_available_options + 1);


		static storing_considered_rolls_with_overall_value *decision_input = \
				new storing_considered_rolls_with_overall_value[9331];
		static storing_considered_rolls_with_overall_value *decision_output = \
				new storing_considered_rolls_with_overall_value[252];


		if (turn_number == 2)
		{
			value_of_roll result;
			if (Rules.is_roll_yahtzee(&(rolls[0])) && available_options[yahtzee])
			{
				result = value_of_roll();
				result.option = yahtzee;

			}
			else
			{
				result = info -> make_choice_during_game(&(rolls[0]), part_three_arr[index], \
					used_available_options, (num_ones(index) == 1), curr_upper_score);


				//now convert from storing_considered_rolls.probability indices to the underlying category
				if (result.option >= 31)
					result.option -= 30;




			}
			if (num_ones(index) == 1) //last turn of the game
			{
				delete [] decision_input;
				delete [] decision_output;
			}
			used_available_options[result.option] = false;

			return vector<int>(1, result.option);
		}

		if (turn_number == 0)
		{
			int rounds_left = freq_ele_in_arr(true, 13, used_available_options + 1);
			if (rounds_left >= 12)
				get_data(index, curr_upper_score);
			else
			{
				clear_out_part_three_arr();

				if (rounds_left > 1)
				{
					Beynesian_Computation computer = Beynesian_Computation(&get_distribution_or_choice, \
							used_available_options, curr_upper_score);

					computer.compute_beynesian_expected_values();

				}

			}
		}

		if (Rules.is_roll_yahtzee(&(rolls[0])))
			return vector<int> (5, rolls[0]);

		get_distribution_or_choice(used_available_options, curr_upper_score, \
				part_three_arr[index], decision_input, decision_output, 2 - turn_number);


		return determine_subset_selection(decision_input, decision_output, rolls);

	}
};

class Human_Player : public Player
{
public:
	//the human makes decisions by probing the view for the selected dice
	vector<int> make_decision(int turn_number, vector<int> rolls)
	{
		if (turn_number < 2)
		{
			bool *is_selected = curr_state.get_is_selected();
			vector<int> selected_dice;
			for (int i = 0; i < 5; i++)
				if (is_selected[i])
					selected_dice.push_back(rolls[i]);
			return selected_dice;
		}
	}
};

//First, waits for the computation thread to catch up (usually, won't be a long wait if -O2). Then inserts scores for CPU_Player into scores[1]
void wait_on_computation_thread(int round_number, CPU_Player *opponent, int scores[2][2])
{

	while (opponent -> get_running_score(round_number)[1] == -1)
		this_thread::sleep_for(chrono::milliseconds(100));

	memcpy(scores[1], opponent->get_running_score(round_number), 2 * sizeof(int));
}

//displays a message about CPU_Player's turn, or updates user-interface on user's turn.
void show_message_when_ready(string curr_message, int round_number, Player *player, Player *opponent, bool is_player_human, \
		int turn_number, vector<int> &curr_rolls)
{
	static queue<Message_Queue_Info> Q;


	if (is_player_human)
	{
		bool available_options[14];
		bool opp_available_options[14];
		for (int i = 1; i < 14; i++)
		{
			available_options[i] = player->isAvailableOption(i);
			opp_available_options[i] = opponent->isAvailableOption(i);
		}
		
		int scores[2][2] = { { player->getCurrUpperScore(), player->getCurrLowerScore() }, {0, 0} };

		//get the score from previous round for opponent
		if (round_number > 0)
		{
			wait_on_computation_thread(round_number - 1, (CPU_Player *)(opponent), scores);


		}
		if (round_number < 13)
		{
			//must get is_selected! Here is the location for the call
			display_rolling_dice(turn_number, scores, available_options, NULL, player->getNumYahtzees(), &(curr_rolls[0]));
			
			curr_state.update_state(turn_number, &(curr_rolls[0]), scores, available_options, NULL, player->getNumYahtzees(), true);
		}
		
		if (turn_number == 2)
		{
			if (round_number < 13)
				player->update_score(curr_state.get_category(), &(curr_rolls[0]));
			scores[0][0] = player->getCurrUpperScore();
			scores[0][1] = player->getCurrLowerScore();

			curr_state.clear_is_selected();

			//now done with all messages for human player
			//print the messages that can. First wait on CPU to catch up to current turn

			wait_on_computation_thread(round_number, (CPU_Player *)(opponent), scores);
			
			
			string message("");
			Lock();
			while (!Q.empty())
			{
				Message_Queue_Info ele = Q.front();
				if (ele.round_number != round_number)
					break;
				message.append(ele.message);
				Q.pop();
			}
			Unlock();

			if (round_number == 13)
			{
				string message_to_send ("");
				message_to_send.append(curr_message);
				message_to_send.append(message);
				curr_state.create_end_of_game(message_to_send.c_str());
				return;
			}

			curr_state.update_state(2, &(curr_rolls[0]), scores, opp_available_options, (char *)(message.c_str()), opponent -> getNumYahtzees());
			curr_state.wait_on_is_done();

			
		}
		else
		{
			curr_rolls = player -> make_decision(turn_number, curr_rolls);
			
			

		}
		return;
	}

	Lock();
	Message_Queue_Info info;
	info.message = curr_message;
	info.round_number = round_number;
	Q.push(info);
	Unlock();

}

//Plays Yahtzee. 
//warning: this function expects one player to be human, and the other to be a CPU_Player or a subclass of one. If otherwise, result is undefined behavior
void play_game(Player *player, Player *opponent)
{
	const bool is_player_human = (dynamic_cast<Human_Player *>(player) != NULL);


	for (int round_number = 0; round_number < 13; round_number ++)
	{
		

		int last_catgr = chance;
		while (!player->isAvailableOption(last_catgr))
			last_catgr--;
		string curr_message = string(" \n"); //so that the top line of the text will be visible
		curr_message.append("available options: \n");

		int num_options_displayed = 0;
		for (int i = 1; i<14; i++)
		{
			if (player->isAvailableOption(i))
			{
				curr_message.append(category_names_without_newlines[i - 1]);
				num_options_displayed++;
				if (i < last_catgr)
				{
					curr_message.append(", ");
					if (num_options_displayed % 5 == 0)
						curr_message.append("\n"); //so all the options fit in the window

				}
			}
		}
		curr_message.append("\n\n");
		if (!is_player_human)
		{
			vector<int> empty = vector<int> ();
			show_message_when_ready(curr_message, round_number, player, opponent, is_player_human, 0, empty);
		}
		curr_message = string();

		vector<int> curr_rolls;
		int category_selected = -1;
		for (int turn_number = 0; turn_number < 3; turn_number ++)
		{

			while (curr_rolls.size() < 5)
				curr_rolls.push_back(ui(re));

			curr_message = string("Dice: \n");
			for (int i=0; i<5; i++)
				curr_message.append(to_str(curr_rolls[i]) + " ");
			curr_message.append("\n");

			if (!is_player_human)
				show_message_when_ready(curr_message, round_number, player, opponent, is_player_human, turn_number, curr_rolls);


			sort(curr_rolls.begin(), curr_rolls.end());

			/*if (is_player_human && turn_number)
				curr_state.update_is_selected(curr_rolls);
*/
			if (turn_number < 2)
			{
				//for the human, make the decision during show_message_when_ready, because we must update the state first
				if (!is_player_human)
					curr_rolls = player -> make_decision(turn_number, curr_rolls);
				string curr_message = string("dice kept: \n");
				for (int dice : curr_rolls)
					curr_message.append(to_str(dice) + " ");
				curr_message.append("\n");
				show_message_when_ready(curr_message, round_number, player, opponent, is_player_human, turn_number, curr_rolls);



				if (curr_rolls.size() == 5)
					turn_number = 2;

			}

			if (turn_number == 2)
			{
				if (is_player_human)
				{
					show_message_when_ready(curr_message, round_number, player, opponent, is_player_human, 2, curr_rolls);

				}
				else
				{
					category_selected = player->make_decision(turn_number, curr_rolls)[0];
					
					player->update_score(category_selected, &(curr_rolls[0]));
					curr_message = "category selected = " + string(category_names_without_newlines[category_selected - 1]) + "\n";
					curr_message.append("upper, lower score = ");
					curr_message.append(to_str(player->getCurrUpperScore() + ((player->getCurrUpperScore() >= 63) ? 35 : 0))); //probably should add 35 here. Try it and pay very close attention to CPU's upper score
					curr_message.append(", ");
					curr_message.append(to_str(player->getCurrLowerScore()));

					show_message_when_ready(curr_message, round_number, player, opponent, is_player_human, 2, curr_rolls);

					((CPU_Player *)(player))->update_running_scores(round_number);

				}
			}
		}


	}

	int final_lower_score = player -> getCurrLowerScore();
	int final_upper_score = player -> getCurrUpperScore() + ((player -> getCurrUpperScore() >= 63) ? 35 : 0);
	string curr_message = string(" \n");
	curr_message.append("final upper score: " + to_str(final_upper_score) + "\n \n");
	curr_message.append("final lower score: " + to_str(final_lower_score) + "\n \n");
	curr_message.append("final score: " + to_str(final_upper_score + final_lower_score) + "\n \n");
	
	vector<int> zeros = vector<int> (5,0);
	show_message_when_ready(curr_message, 13, player, opponent, is_player_human, 2, zeros);

}
