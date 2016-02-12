#include "View.hpp"
#include "Aux_GL.cpp"
#include "Rules.hpp"


enum { normal, Ignore, for_exiting };

template <typename T>
extern vector<T> vec_remove(T ele, vector<T> V);

//Player's name
char human_name[100];

vector<vector<float>> dice_coords;

bool can_reroll;

State::State(void(*lock_funct)(), void(*unlock_funct)())
{
	this->lock_funct = lock_funct;
	this->unlock_funct = unlock_funct;
	state_turn_number = 0;
	is_done = false;
	for (int i = 0; i < 5; i++)
		is_selected[i] = false;
	for (int i = 1; i < 14; i++)
		available_options[i] = true;
	state_category = 0;
	text_to_display = NULL; //text_to_display is NULL iff is human's turn
	is_view_blocked = false;
	mouse_state = normal;
	num_yahtzees = 0;

	
}


State::~State()
{
	Lock();
	delete[] text_to_display;
	text_to_display = NULL;
	Unlock();
}

//retrieves information about the state. All parameters except text_to_display may be null, in which case no data is extracted.
bool *State:: get_info_for_view(int *turn_num, int rolls[5], int scores[2][2], bool *is_done, bool available_options[14], char **text_to_display, int *num_yahtzees)
{
	lock_funct();

	if (turn_num != NULL)
		*turn_num = state_turn_number;

	if (rolls != NULL)
		memcpy(rolls, state_rolls, 5 * sizeof(int));
	
	if (scores != NULL)
	{
		for (int i = 0; i < 2; i++)
		{
			scores[i][0] = state_scores[i].first;
			scores[i][1] = state_scores[i].second;
		}
	}

	if (is_done != NULL)
		*is_done = this->is_done;
	
	if (available_options != NULL)
		memcpy(available_options + 1, this->available_options + 1, 13 * sizeof(bool));
	
	if (num_yahtzees != NULL)
		*num_yahtzees = this-> num_yahtzees;

	(*text_to_display) = this->text_to_display;
	
	static bool is_selected [5];
	memcpy(is_selected, this->is_selected, 5 * sizeof(bool));
	unlock_funct();
	return is_selected;
}

//Waits until the "Done" button is pressed, and then marks it as unpressed the next time this function is called
void State::wait_on_is_done()
{
	while (!this->is_done)
		this_thread::sleep_for(chrono::milliseconds(100));
	is_done = false; //for whatever next user input waiting on

}

//Allows the human-player thread to update all members of state which are intended to have setters. This operation is performed in a critical section so that a single
//moment in the game can be captured
void State:: update_state(int turn_num, int rolls[5], int scores[2][2], bool available_options[14], const char *text_to_display, int num_yahtzees, bool wait_on_done)
{
	lock_funct();
	memcpy(state_rolls, rolls, 5 * sizeof(int));
	state_turn_number = turn_num;
	memcpy(this->available_options + 1, available_options + 1, 13 * sizeof(bool));
	store_str_in_dynamic_str(text_to_display, &(this->text_to_display));
	for (int i = 0; i < 2; i++)
	{
		state_scores[i].first = scores[i][0];
		state_scores[i].second = scores[i][1];
	}
	if (turn_num < 2)
		state_category = 0;

	this->num_yahtzees = num_yahtzees;

	unlock_funct();

	//now that the state is correct, we can unblock the view
	is_view_blocked = false;
	

	if (wait_on_done)
	{
		wait_on_is_done();
	}

	force_view_redisplay();


}

//Creates the conditions for the end of the game: displaying of the final scores, using the "done" button to mean exit
void State::create_end_of_game(const char *message)
{
	
	store_str_in_dynamic_str(message, &(this->text_to_display));
	mouse_state = for_exiting;
	is_view_blocked = false;
	force_view_redisplay();

}

//Technical function which uses some values from the current state, combined with data from mouse clicks, to update the state.
void State::update_state_based_on_state(void *data, function<void(void *data, int turn_number, int rolls[5], \
	int &category, bool &is_done, bool is_selected[5], bool available_options[14], char *text_to_display)> determine_state)
{
	lock_funct();
	determine_state(data, state_turn_number, state_rolls, state_category, is_done, is_selected, available_options, text_to_display);
	unlock_funct();
}

//Returns which dice the user has selected to keep
bool * State::get_is_selected()
{
	lock_funct();
	static bool cpy_is_selected[5];
	memcpy(cpy_is_selected, is_selected, 5 * sizeof(bool));
	unlock_funct();
	return cpy_is_selected;
}

//When the dice are rerolled, updates the is_selected array so that the same dice are represented in their new locations within curr_rolls
void State::update_is_selected(vector<int> &new_rolls)
{
	int index_correspondence[5];
	int index = 0;
	for (int i = 0; i < 5; i++)
		if (is_selected[i])
		{
			index_correspondence[i] = find(new_rolls.begin() + index, new_rolls.end(), state_rolls[i]) - new_rolls.begin();
			index = index_correspondence[i] + 1;
		}

	bool result[5] = { false };
	for (int i = 0; i < 5; i++)
		if (is_selected[i])
			result[index_correspondence[i]] = true;
	memcpy(is_selected, result, 5 * sizeof(bool));
}

//At the end of a round, none of the die are selected by the user
void State::clear_is_selected()
{
	for (int i = 0; i < 5; i++)
		is_selected[i] = false;
}

//returns category selected by user
int State::get_category()
{
	return state_category;
}

void State::clear_text()
{
	text_to_display = NULL;
}

//Creates the dice, the category buttons, and the score pane
void create_frame(vector<const char *> category_names, vector<vector<float>> &dice_coords, bool available_options[14], char *text_to_display, int turn_num, int num_yahtzees, int rolls[5], int scores[2][2], bool is_selected [5])
{
	float colors[3];


	//creates dice as cubes with dots
	for (int i = 0; i < 3; i++)
		colors[i] = 1; //white
	dice_coords = vector<vector<float>>(5);
	float left_first_die = Dimensions.window_width / (6.0 * sqrt(2));
	for (int i = 0; i < 5; i++)
	{
		dice_coords[i].push_back(left_first_die + i * Dimensions.window_width / 6); //x
		dice_coords[i].push_back(Dimensions.window_height * 2 / 3); //y
		dice_coords[i].push_back(Dimensions.window_height / 6); //edge length
		if (text_to_display == NULL)
			create_cube_with_border(dice_coords[i][0], dice_coords[i][1], dice_coords[i][2], colors);
		//if computer's turn, we still need to calculate dice_coords, just not draw the dice to the window
	}

	//if user's turn, display the categories remaining
	if (text_to_display == NULL)
	{

		colors[0] = 0; colors[1] = 1; colors[2] = 0; //green
		float red[3] = { 1, 0, 0 };
		float black[3] = { 0, 0, 0 };
		//create the squares for each rectangle
		float delta_x = Dimensions.window_width / 5;
		float delta_y = Dimensions.window_height / 6;

		for (int i = 0; i < 13; i++)
		{
			int row = i % 3;
			int col = i / 3;
			create_rectangle_with_border(col * delta_x, row * delta_y, delta_y, delta_x, colors);
			if (available_options[i + 1])
			{
				float input_coords[2] = { (col + 0.1f) * delta_x, (row + 0.5f) * delta_y };
				if (i + 1 == curr_state.get_category())
					show_text(input_coords[0], input_coords[1], category_names[i], red, 1, false);
				else
					show_text(input_coords[0], input_coords[1], category_names[i], black, 1, false);

				//now display score would get by choosing a category
				if (turn_num == 2)
				{
					if ((i + 1 != yahtzee) || (num_yahtzees == 0) || Rules.is_roll_yahtzee(rolls))
					{
						input_coords[1] += TextInfo(Dimensions.window_width).gap * (freq_ele_in_arr('\n', strlen(category_names[i]), (char *)(category_names[i])) + 1);
						int score = Rules.num_pts_for_roll(i + 1, rolls, num_yahtzees, available_options);
						if ((i + 1 <= sixes) && (scores[0][0] < 63) && (scores[0][0] + score >= 63))
							score += 35; //for the bonus
						show_text(input_coords[0], input_coords[1], to_str(score).c_str(), ((i + 1 == curr_state.get_category()) ? red : black), 1);
					}
				}

			}

		}

		//create a message instructing users
		float white[3] = { 1, 1, 1 };
		float coords_for_instructions[2] = { 30, Dimensions.window_height * 0.83f };
		if (turn_num < 2)
		{
			if (turn_num == -1)
				show_text(coords_for_instructions[0], coords_for_instructions[1], "Rolling", white);
			else if (freq_ele_in_arr(true,5, is_selected) == 5)
				show_text(coords_for_instructions[0], coords_for_instructions[1], "Click dots again to deselect, or GO to select scoring category.", white);

			else
				show_text(coords_for_instructions[0], coords_for_instructions[1], "Select dice to keep by clicking the dots. Click again to deselect.", white);
		
		
		}
		else
			show_text(coords_for_instructions[0], coords_for_instructions[1],     "Select a scoring category above, and then click GO.", white);

	}
	else
	{
		//display information about CPU's turn

		show_text(0.05 * Dimensions.window_width, 0.05 * Dimensions.window_height, text_to_display, colors, get_num_lines_before_sleep(text_to_display));
			//indicates that the text has been shown by blocking view from redrawing it
		curr_state.is_view_blocked = true;

	}



	colors[0] = 0; colors[1] = 255; colors[2] = 255; //light blue
	create_rectangle(0, Dimensions.window_height * 0.85, Dimensions.window_height  * 0.15, Dimensions.window_width, colors);

	//create a button for done
	float green[3] = { 0, 1, 0 }; float red[3] = { 1, 0, 0 };
	create_circle(0.9 * Dimensions.window_width, 0.91 * Dimensions.window_height, 0.06 * Dimensions.window_height, green);
	float coords[2] = { .87f * Dimensions.window_width, .91f * Dimensions.window_height };
	
	//shows message on the "done" button, depending on the state

	//if last round and CPU made its turn
	if ((text_to_display != NULL) && ((num_yahtzees + freq_ele_in_arr(false, 13, available_options + 1)) >= 12))
	{
		//if showing final score
		show_text(coords[0], coords[1], (curr_state.get_mouse_state() == for_exiting) ? "Quit" : "Done", red);
	}
	else if (text_to_display != NULL)
		show_text(coords[0], coords[1], "Roll", red);
	else
		show_text(coords[0], coords[1], ((turn_num < 2) && (freq_ele_in_arr(true, 5, is_selected) < 5)) ? "Roll" : "GO!", red);
	
}

//returns a list of points at which to draw circles at to represent holes on a die
vector<pair<float,float>> Dice::list_pos_dots(int roll, pair<float,float> p1, pair<float, float> p2, pair<float,float> p3)
{
	vector<int> dot_pos;
	switch (roll)
	{
	case 1: 
		dot_pos.push_back(4); break;
	case 2:
		dot_pos.push_back(0); dot_pos.push_back(8); break;
	case 3:
		dot_pos.push_back(0); dot_pos.push_back(4); dot_pos.push_back(8); break;
	case 4:
		dot_pos.push_back(0); dot_pos.push_back(2); dot_pos.push_back(6); dot_pos.push_back(8); break;
	case 5:
		dot_pos.push_back(0); dot_pos.push_back(2); dot_pos.push_back(4); dot_pos.push_back(6); dot_pos.push_back(8); break;
	case 6:
		for (int i = 0; i < 9; i++)
			if ((i / 3) != 1)
				dot_pos.push_back(i);
		break;
	}
	float dist_p1_p2 = dist(p1, p2);
	float dist_p1_p3 = dist(p1, p3);
	vector<pair<float,float>> result;
	
	float partitions[2][3] = {{ 0.2f * dist_p1_p2, 0.5f * dist_p1_p2, 0.8f * dist_p1_p2}, {0.2f * dist_p1_p3, 0.5f * dist_p1_p3, 0.8f * dist_p1_p3}};
	
	float unit_vec_x[2] = { (p2.first - p1.first) / dist_p1_p2, (p2.second - p1.second) / dist_p1_p2 };
	float unit_vec_y[2] = { (p3.first - p1.first) / dist_p1_p3, (p3.second - p1.second) / dist_p1_p3 };


	for (int ele : dot_pos)
	
	result.push_back(pair < float, float > {p1.first + partitions[0][ele / 3] * unit_vec_x[0] + partitions[1][ele % 3] * unit_vec_y[0], \
		p1.second + partitions[0][ele / 3] * unit_vec_x[1] + partitions[1][ele % 3] * unit_vec_y[1]});
	
	return result;
}

//shows all the dots on a single die with value roll. The top left corner of the face on which to draw must be provided as parameters (these are prior to rotation)
void Dice::show_die(float center_x, float center_y, float side_len, int roll, float color[3], int die_index)
{
	static float smaller_dim = min(Dimensions.window_width, Dimensions.window_height);
	float radius = smaller_dim / 120;
	//a vertex on the face on which to display dice
	pair<float, float> vertex_1 = pair<float,float> { center_x, center_y - 0.75 * side_len };
	
	pair<float, float> vertex_2 = pair < float, float > {center_x + (sqrt(2) / 2) * 0.75 * side_len, center_y - (sqrt(2) / 2) * 0.75 * side_len};
	pair<float, float> vertex_3 = pair < float, float > {center_x - (sqrt(2) / 2) * 9 * side_len / 8, center_y - (sqrt(2) / 2) * 0.45 * side_len};


	float top_left[2] = { vertex_1.first, vertex_1.second };
	vector<pair<float, float>> points = list_pos_dots(roll, vertex_1, vertex_2, vertex_3);
	for (pair<float, float> point : points)
	{
		pos_of_dots.push_back(vector < float > {point.first, point.second, radius, (float)(die_index)});
		create_circle(point.first, point.second, radius, color);
	}
}

//Updates which dice are to be shown and in what order
void Dice::update(int rolls[5], vector<vector<float>> &dice_coords, bool is_selected[5])
{
	pos_of_dots.clear();
	float black[3] = { 0 };
	float red[3] = { 1, 0, 0 };
	for (int i = 0; i < 5; i++)
	{
		if (is_selected[i])
			show_die(dice_coords[i][0], dice_coords[i][1], dice_coords[i][2], rolls[i], red, i);
		else
			show_die(dice_coords[i][0], dice_coords[i][1], dice_coords[i][2], rolls[i], black, i);

	}//issue is there are for the centers, not for the top-left pts
	//also, must figure out translating in third dimension
}


Score_Pane::Score_Pane(string human_name)
{
	this->human_name = human_name;
}

//Returns the string to be shown in the panel.
char *Score_Pane::assemble_string(bool is_human, int curr_upper_score, int curr_lower_score)
{
	char *string_to_show = new char[(is_human) ? (human_name.length() + 20) : 28];
	char *curr_pos = string_to_show;
	if (curr_upper_score >= 63)
		curr_upper_score += 35; //just what is shown, not stored internally
	if (is_human)
		curr_pos += sprintf(curr_pos, "%s: ", human_name.c_str());
	else
		curr_pos += sprintf(curr_pos, "%s: ", "Computer");
	curr_pos += sprintf(curr_pos, "%d, ", curr_upper_score);
	curr_pos += sprintf(curr_pos, "%d", curr_lower_score);
	
	return string_to_show;
}

//Shows the scores
void Score_Pane::update(bool is_player_human, int curr_upper_score, int curr_lower_score, int round_number)
{

	float colors[3] = { 0, 0, 0 };
	char *string_to_show = assemble_string(is_player_human, curr_upper_score, curr_lower_score);
	float coords[2] = { 30, (float) (0.9 + ((is_player_human) ? 0 : .05)) * Dimensions.window_height};
	show_text(coords[0], coords[1], string_to_show, colors);
	delete string_to_show;
	
	if (round_number != -1)
	{
		string display_round_num = string("Round ") + to_str(round_number); 
		show_text(0.5 * Dimensions.window_width, coords[1], display_round_num.c_str(), colors);
	}

}

//------------------------VIEW FUNCTIONS------------------------


//Idle callback. Signals that glut is not currently drawing the view. Use: if another thread needs to wait to make another request to redisplay
void signal_idle()
{
	if (curr_state.get_mouse_state() && (curr_state.is_view_blocked == Ignore))
	{
		can_reroll = true;
		curr_state.is_view_blocked = false;
	}
}

void display_rolling_dice(int turn_num, int scores[2][2], bool available_options[14], const char *text_to_display, int num_yahtzees, int curr_rolls[5])
{
	if (freq_ele_in_arr(true, 5, curr_state.get_is_selected()) == 5)
		return;

	while (Dimensions.window_height == 0)
		this_thread::sleep_for(chrono::milliseconds(100));
	curr_state.set_mouse_state(Ignore);
	vector<int> cpy_curr_rolls(5,0);
	char *null_text = NULL;
	curr_state.get_info_for_view(NULL, &(cpy_curr_rolls[0]), NULL, NULL, NULL, &null_text, NULL); //get the prior rolls
	can_reroll = true;
	for (int i = 0; i < 10; i++)
	{
		bool *is_selected = curr_state.get_is_selected();

		vector<int> auxillary = cpy_curr_rolls;

		if (i < 9)
		{
			for (int j = 0; j < 5; j++)
				if (!is_selected[j])
					auxillary[j] = ui(re);
		}
		else
		{
			memcpy(&(auxillary[0]), curr_rolls, 5 * sizeof(int));
			curr_state.update_is_selected(auxillary);
		}

		cpy_curr_rolls = auxillary;

		curr_state.update_state(((i < 9) ? -1 : turn_num), &(cpy_curr_rolls[0]), scores, available_options, text_to_display, num_yahtzees);
		
		this_thread::sleep_for(chrono::milliseconds(300));
		while (!can_reroll)
			this_thread::sleep_for(chrono::milliseconds(25));
		can_reroll = false;
		
	}
	curr_state.set_mouse_state(normal);
	curr_state.is_view_blocked = true; //await the final updating of the state
}

//Mouse callback function. It uses the position to find the dice selected or the category chosen. 
void onClick(int button, int state, int x, int y)
{
	int curr_mouse_state = curr_state.get_mouse_state();

	if (curr_mouse_state == Ignore)
		return;

	static bool got_yahtzee = false;

	if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
		return; //if not pressing down on the left part of the mouse, not a signal to respond to
	
	if (glutGet(GLUT_WINDOW_WIDTH) > glutGet(GLUT_WINDOW_HEIGHT))
		x = (int) (x * Dimensions.window_width / glutGet(GLUT_WINDOW_WIDTH));
	else if (glutGet(GLUT_WINDOW_WIDTH) < glutGet(GLUT_WINDOW_HEIGHT))
		y = (int) (y * Dimensions.window_height / glutGet(GLUT_WINDOW_HEIGHT));

	int coords[2] = { x, y };

	curr_state.update_state_based_on_state(&coords, [curr_mouse_state](void *data, int turn_num, int rolls[5], int &category, bool &is_done, bool is_selected[5], bool available_options[14], char *text_to_display)
	{


		int x = (*((int((*)[2]))(data)))[0];
		int y = (*((int((*)[2]))(data)))[1];


		if (dist(pair < float, float > {x, y}, pair < float, float > {0.9 * Dimensions.window_width, 0.91 * Dimensions.window_height}) <= .06* Dimensions.window_height)
		{
	
			if (curr_mouse_state  == for_exiting)
				glutLeaveMainLoop();

			if ((turn_num != 2) || (category != 0))
				// clicks within the circle with "done"
			{
				curr_state.is_view_blocked = true; //now not allowed to draw the view until the state is updated


				curr_state.clear_text(); //this is essential to prevent redisplaying of the CPU's turn

				if (turn_num == 2 && category == yahtzee)
					got_yahtzee = true;

				is_done = true;

			}
		}
		else if (curr_mouse_state == for_exiting)
			return;
		else if (text_to_display != NULL)
			return;
		else if (turn_num == 2)
		{
			if ((x < 0) || (y < 0) || (x > Dimensions.window_width) || (y > Dimensions.window_height) || (x > 0.8 * Dimensions.window_width && y > Dimensions.window_height / 6))
				return;
			//now get the category
			int col = (int) (x * 5 / Dimensions.window_width);
			int row = (int) (y * 6 / Dimensions.window_height);
			if (available_options[col * 3 + row + 1])
				if ((col * 3 + row + 1 != yahtzee) || !got_yahtzee || Rules.is_roll_yahtzee(rolls))
					category = col * 3 + row + 1;
		}

		else
		{
			//find any die selected. If rolls contains it, remove the element. Otherwise, add.
			//To make things simpler, user must press on a dot, or won't register.
			
			vector<vector<float>> dot_pos = Dice().get_pos_of_dots();
			for (vector<float> dot : dot_pos)
			{
				if (dist(pair < float, float > {x, y}, pair < float, float > {dot[0], dot[1]}) <= dot[2])
				{
					is_selected[(int)(dot[3])] = !(is_selected[(int)(dot[3])]);
					break;
				}
			}
		}
		
	});
	
	if (curr_mouse_state == normal)
		force_view_redisplay();
}

//Displays the view to the window, getting information from curr_state.
void displayView()
{

	if (curr_state.is_view_blocked)
	{
		char *text;
		curr_state.get_info_for_view(NULL, NULL, NULL, NULL, NULL, &text, NULL);
		if (text != NULL)
			return; //there can be only one view display on CPU turn that ever is allowed to proceed. The same applies if the dice is being rolled
		while (curr_state.is_view_blocked)
			this_thread::sleep_for(chrono::milliseconds(100));

	}

	glClear(GL_COLOR_BUFFER_BIT);

	Dimensions.window_width = (float)(min(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT)));
	Dimensions.window_height = Dimensions.window_width;


	bool available_options[14];
	int rolls[5]; int scores[2][2];
	char *text_to_display;
	int turn_num;
	int num_yahtzees;
	bool *is_selected = curr_state.get_info_for_view(&turn_num, rolls, scores, NULL, available_options, &text_to_display, &num_yahtzees);
	//get the current state
	
	


	create_frame(category_names, dice_coords, available_options, text_to_display, turn_num, num_yahtzees, rolls, scores, is_selected);


	if (text_to_display == NULL)
	{
		Dice dice = Dice();
		dice.update(rolls, dice_coords, is_selected);
	}


	Score_Pane pane = Score_Pane(string(human_name));
	
	
	pane.update(true, scores[0][0], scores[0][1], freq_ele_in_arr(false, 13, available_options + 1) + num_yahtzees + ((text_to_display == NULL) ? 1 : 0));
	pane.update(false, scores[1][0], scores[1][1]);

	glFlush();

	if (curr_state.get_mouse_state() == Ignore)
		curr_state.is_view_blocked = true;
	
}
