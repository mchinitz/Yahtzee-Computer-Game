class TextInfo
{
public:
	void *Font;
	int gap;

	TextInfo(float window_dimension)
	{
		if (window_dimension >= 650)
		{
			Font = GLUT_BITMAP_TIMES_ROMAN_24;
			gap = 24;
		}

		else if (window_dimension >= 550)
		{
			Font = GLUT_BITMAP_HELVETICA_18;
			gap = 18;
		}

		else if (window_dimension >= 400)
		{
			Font = GLUT_BITMAP_HELVETICA_12;
			gap = 12;
		}

		else if (window_dimension >= 300)
		{
			Font = GLUT_BITMAP_TIMES_ROMAN_10;
			gap = 10;
		}

		else
		{
			cout << "Error, window provided by the window manager is too small" << endl;
			exit(EXIT_FAILURE);
		}
	}

};
