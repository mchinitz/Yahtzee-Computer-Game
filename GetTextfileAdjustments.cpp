void get_data(int options_index, int upper_score)
{
	FILE *fp = fopen("SavedAdjustments", "r");

	if (fp == NULL)
	{
		cout << "Error, file not found" << endl;
		exit(EXIT_FAILURE);
	}
	char *desired_data_header = (char *) calloc(15, sizeof(char));
	int num_char_so_far = sprintf(desired_data_header, "%d ", options_index);
	sprintf(desired_data_header + num_char_so_far, "%d\n", upper_score);
	//const char *desired_data_header = (to_string(options_index) + " " + to_string(upper_score) + "\n").c_str();
	//no reason to think that the memory address immediately following the end of this string (the null char)
	//can't be used by other thread, or maybe by something else.

	char actual_line [256];

	while (true)
	{
		if (fgets(actual_line, 256, fp) == NULL)
		{
			cout << "Error, failed to find distribution " << options_index << " " << upper_score << endl;
			free(desired_data_header);
			exit(EXIT_FAILURE);
		}

		if (strcmp(actual_line, desired_data_header) == 0)
			break;
	}
	free(desired_data_header);
	for (int i=0; i<13; i++)
	{
		fgets(actual_line, 256, fp);
		char *s = actual_line;
		part_three_arr[options_index][i].score_independent_from_freq = strtod(s, &s);
		for (int j=0; j<4; j++)
			part_three_arr[options_index][i].freq_dependent_scores[j] = strtod(s, &s);
	}
	fclose(fp);
}
