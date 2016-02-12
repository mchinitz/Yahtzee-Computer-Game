#ifndef INC_VIEW_HPP
#define INC_VIEW_HPP

extern void Lock();
extern void Unlock();
#include <string>
#include <vector>
#include <functional>

//The category names with new lines inserted for fortmatting reasons
vector<const char *> category_names = { "ones", "twos", "threes", "fours", "fives", "sixes", "three of a \nkind", \
"four of a \nkind", "full house", "small strait", "large strait", "yahtzee", "chance" };

//Consistent with the State Pattern, this class is used to update the view based on the state of the game.
//The state is not controlled by the view, but is managed by the human-player thread. Thus, the state functions
//as a callback to OpenGL (it explicitly requests for the view to be redisplayed). All of the variables (except for the functions
//and state scores) pertain specifically to the user. State scores is a 2x2 matrix. Row 1 = user, row 2 = CPU. Column 1 = upper scores, column 2 = lower scores.
class State
{
	void(*lock_funct)();
	void(*unlock_funct)();
	int state_turn_number;
	int state_rolls[5];
	pair<int, int> *state_scores = get_2D_arr_zeros();
	int state_category;
	bool is_done; //reflects the moment when the user presses the "Done" button. 
	bool is_selected [5]; //is the ith die selected to be kept by the user
	bool available_options[14];
	char *text_to_display; //primarely for displaying information about the computer's turn
	int mouse_state; //a flag that is set to designate whether/how to respond to mouse clicks. If for_exiting, exit if clicks on "Done" and ignore otherwise
	int num_yahtzees;

	pair<int, int> *get_2D_arr_zeros()
	{
		static pair<int, int> result[2];
		for (int i = 0; i < 2; i++)
			result[i] = pair < int, int > {0, 0};
		return result;
	}

public:

	bool is_view_blocked; //at the end of the CPU's turn, we must force the view to not update until the human player thread is ready and can update the state correctly

	~State();
	State(void(*lock_funct)(), void(*unlock_funct)());
	
	//functions for getting and setting the state:
	
	//in a single critical section, from the view, fetch the turn number, determine the rolls and scores, and store those values in curr_state. Returns is_selected
	bool *get_info_for_view(int *turn_num, int rolls[5], int scores[2][2], bool *is_done, bool available_options[14], char **text_to_display, int *num_yahtzees);


	//Allows the human-player thread to notify the view of the game progress.
	void update_state(int turn_num, int rolls[5], int scores[2][2], bool available_options[14], const char *text_to_display, int num_yahtzees, bool wait_on_done = false);
	
	void update_state_based_on_state(void *data, function<void(void *data, int turn_number, int rolls[5], \
		int &category, bool &is_done, bool is_selected[5], bool available_options[14], char *text_to_display)> determine_state);

	//Waits until the "Done" button is pressed, and then marks it as unpressed the next time this function is called
	void wait_on_is_done();

	//Returns category selected by the user, or zero if no category is selected yet.
	int get_category();

	//Returns the dice the user has currently selected.
	bool *get_is_selected();

	//When the dice are rerolled, updates the is_selected array so that the same dice are represented in their new locations within curr_rolls
	void update_is_selected(vector<int> &new_rolls);

	//Sets each entry of state.is_selected to false
	void clear_is_selected();

	//Sets state.text_to_display = NULL
	void clear_text();

	//Updates the state to reflect the end of the game
	void create_end_of_game(const char *message);

	int get_mouse_state()
	{
		return mouse_state;
	}

	void set_mouse_state(int mouse_state)
	{
		if (mouse_state > 2)
			throw exception();
		this->mouse_state = mouse_state;
	}
};

State curr_state(Lock, Unlock);

//Contains the methods to display the dice
class Dice
{
	static vector<vector<float>> pos_of_dots; //defined so each element has 4 numbers: two coordinates, radius of dot, and dice index	
	vector<pair<float, float>> list_pos_dots(int roll, pair<float, float> p1, pair<float,float> p2, pair<float,float> p3);

	//shows all the dots on a single die with value roll. The top left corner of the face on which to draw must be provided as parameters (these are prior to rotation)
	void show_die(float center_x, float center_y, float side_len, int roll, float color[3], int die_index);
public:

	//Updates which dice are to be shown and in what order
	void update(int rolls[5], vector<vector<float>> &dice_coords, bool is_selected [5]);

	vector<vector<float>> get_pos_of_dots()
	{
		return pos_of_dots;
	}
};

vector<vector<float>> Dice::pos_of_dots = Dice::pos_of_dots; //otherwise, linker error

//For showing the scores
class Score_Pane
{
	string human_name;
	bool CPU_available_options;

	char *assemble_string(bool is_human, int curr_upper_score, int curr_lower_score);

public:
	Score_Pane(string human_name);

	//Updates the pane to reflect the scores specified as arguments
	void update(bool is_player_human, int curr_upper_score, int curr_lower_score);

};

//Creates the dice, the category buttons, and the score pane. The inputs (except the first two) reflect the current state. Dice_coords are locations for drawing dice to on the frame, and are initialized by this function
void create_frame(vector<const char *> category_names, vector<vector<float>> &dice_coords, bool available_options[14], char *text_to_display, int turn_num, int num_yahtzees, int rolls[5], int scores[2][2], bool is_selected[5]);

//Mouse callback function which responds to the user selecting dice to keep, choosing a category, or clicking on the "Done" button. Updates the state and notifies the view to redraw itself
void onClick(int button, int state, int x, int y);

//Draws the view. Used for the glutMainLoop function.
void displayView();


#endif
