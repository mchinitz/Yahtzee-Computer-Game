#include "YahtzeeDistributions.hpp"
#include <vector>
#include <math.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <string.h>

//Auxillary, general-purpose functions

template <typename T>
T sum_array(int length, T A[])
{
	T sum = 0;
	for (int i=0; i<length; i++)
		sum += A[i];
	return sum;
}



template <typename T>
int freq_ele_in_arr(T ele, int length, T A[])
{
	int freq = 0;
	for (int i=0; i<length; i++)
		if (A[i] == ele)
			freq ++;
	return freq;
}

template <typename T>
T max_array(int len, T A[])
{
	T Max = A[0];
	for (int i=1; i<len; i++)
		Max = max(Max, A[i]);
	return Max;
}

int convert_base_2_to_10(int len, bool A[])
{
	int result = 0;
	for (int i=0; i<len; i++)
		result += (A[i]) * powers_of_two_array[len-i-1];
	return result;
}

vector<bool> convert_key_to_subset(int key)
{
	vector<bool> curr_categories;
	int index = pow(2, floor(log2(key)));
	int prev_result = key, remainder = key;
	while (index > 0)
	{
		prev_result = remainder / index;
		remainder %= index;
		curr_categories.push_back(prev_result);
		index /= 2;
	}
	while (curr_categories.size() < 13)
		curr_categories.insert(curr_categories.begin(), 0);
	return curr_categories;
}

//raises an assertion error if the sum of the elements of A is zero
void normalize_arr(int len, double A[])
{
	double sum = sum_array(len, A);
	assert(sum != 0);
	double quotient = 1 / sum;
	for (int i=0; i<len; i++)
		A[i] *= quotient;
}


template <typename T>
bool has_ele(T ele, int len, T A[])
{
	for (int i=0; i<len; i++)
		if (A[i] == ele)
			return true;
	return false;
}


//returns sum(A[i] * 6^i from i=0 to len - 1)
int find_powered_key(int len, int A[])
{
	int key = 0;
	for (int i=0; i<len; i++)
		key += powers_of_six_array[i] * A[i];
	return key;
}

string to_str(int x)
{
	ostringstream aux_stream;
	aux_stream << x;
	return aux_stream.str();
}

//for OpenGL

template <typename T>
vector<T> vec_remove(T ele, vector<T> V)
{
	vector<T> result;
	for (T element : V)
		if (element != ele)
			result.push_back(element);
	return result;
}

void push_next_string(vector<char *> &lines, const char *text, int index, int start)
{
	lines.push_back(new char[index - start + 1]);
	memcpy(lines[lines.size() - 1], text + start, sizeof(char) * (index - start));
	lines[lines.size() - 1][index - start] = '\0';
	
}

//note: all elements are dynamically allocated
vector<char *> get_lines(const char *text)
{
	vector<char *> lines;
	if (text[0] == '\0')
		return lines;
	int start = 0;
	int index = 0;
	while (text[index] != '\0')
	{
		if (text[index] == '\n')
		{
			push_next_string(lines, text, index, start);
			start = index;
		}
		index++;
	}
	//now extract the last element
	if (text[strlen(text) - 1] != '\n')
		push_next_string(lines, text, index, start);
	
	return lines;
}

//given a string that has two consecutive newline characters, returns the line number following the second of the newline characters
int get_num_lines_before_sleep(char *text)
{
	int line_num = 0;
	int len = (int) strlen(text);
	int num_consecutive_new_lines = 0;
	for (int i = 0; i < len; i++)
	{
		if (text[i] == '\n')
		{
			line_num++;
			num_consecutive_new_lines++;
			if (num_consecutive_new_lines == 2)
				return line_num;
		}
		else
			num_consecutive_new_lines = 0;
	}

	return strlen(text); //i.e. never wait
}

//copies a string into a string stored on the heap
void store_str_in_dynamic_str(const char*source, char **dest)
{
	delete[] (*dest);
	(*dest) = NULL;
	if (source == NULL)
		return;
	(*dest) = new char[sizeof(char) * (strlen(source) + 1)];
	strcpy(*dest, source);
}
