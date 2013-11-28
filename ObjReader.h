#include <string>
#include <vector>
#include "Triangle.h"
#include "Vect.h"
#include "Color.h"
#include "Object.h"
#include <fstream>
#include <iostream>

using namespace std;
class ObjReader
{
	string file_name;
	Color object_color;
	float refraIdxValue;
	float x_offset,y_offset,z_offset;
	vector <Vect> points;
	vector <Vect> normals;
public:
	ObjReader(string file_name,Color color, float refraIdxValue,float x_offset,float y_offset,float z_offset)
	{
		this->file_name=file_name;
		this->object_color=color;
		this->refraIdxValue=refraIdxValue;
		this->x_offset=x_offset;
		this->y_offset=y_offset;
		this->z_offset=z_offset;
	}

	ObjReader()
	{

	}

	void SetPara(string file_name,Color color, float refraIdxValue,float x_offset,float y_offset,float z_offset)
	{
		this->file_name=file_name;
		this->object_color=color;
		this->refraIdxValue=refraIdxValue;
		this->x_offset=x_offset;
		this->y_offset=y_offset;
		this->z_offset=z_offset;
	}

	bool ReadContent(vector<Object*>* scence_objects)
	{
		fstream input(file_name,ios::in);
		string head;
		float value_one,value_two,value_three;
		char* string_one;
		char* string_two;
		char* string_three;
		string_one=new char[20];
		string_two=new char[20];
		string_three=new char[20];
		if(!input)
			return false;

		while(!input.eof())
		{
			input>>head;

			if(head=="v")
			{
				input>>value_one>>value_two>>value_three;
				Vect point(value_one+x_offset,value_two+y_offset,value_three+z_offset);
				points.push_back(point);
			}
			else if(head=="f")
			{
				input>>string_one>>string_two>>string_three;
				int normal_x,normal_y,normal_z;

				value_one=atoi(strtok(string_one,"/"))-1;
				strtok(NULL,"/");
				normal_x=atoi(strtok(NULL,"/"))-1;

				value_two=atoi(strtok(string_two,"/"))-1;
				strtok(NULL,"/");
				normal_y=atoi(strtok(NULL,"/"))-1;

				value_three=atoi(strtok(string_three,"/"))-1;
				strtok(NULL,"/");
				normal_z=atoi(strtok(NULL,"/"))-1;

				Triangle* triangle=new Triangle(points.at(value_one),points.at(value_two),points.at(value_three),object_color,refraIdxValue);
				triangle->setNormals(normals.at(normal_x),normals.at(normal_y),normals.at(normal_z));
				scence_objects->push_back(triangle);
			}
			else if(head=="vn")
			{
				input>>value_one>>value_two>>value_three;
				Vect normal(value_one,value_two,value_three);
				normals.push_back(normal);
			}
		}
		input.close();
		points.clear();
		return true;
	}
};