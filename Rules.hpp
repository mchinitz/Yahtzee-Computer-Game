#ifndef INC_RULES_HPP
#define INC_RULES_HPP

//This class adheres strictly to the rules of the game, impartial to the decision algorithm. Returns information about a roll.
static struct rules
{

	bool is_roll_yahtzee(int rolls[5]);

	//given a roll and other information about a player, returns how many points the player would earn by choosing a particular category.
	//This is useful not only for updating scores, but also for displaying the scores that the user would get if he or she chose a given category.
	bool is_valid_option(int category, int rolls[5], bool available_options[NUM_CATEGORIES + 1]); 
	
	//Returns the number of points a specific roll of the dice would earn if a specific category is chosen. Makes sure that the choice is not already used up by the player on a prior round.
	int num_pts_for_roll(int category, int rolls[5], int num_yahtzees, bool available_options[NUM_CATEGORIES + 1]);

}  Rules;

#endif