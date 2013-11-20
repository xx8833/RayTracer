#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <limits>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "Vect.h"
#include "Ray.h"
#include "Camera.h"
#include "Color.h"
#include "Light.h"
#include "Object.h"
#include "Source.h"
#include "Sphere.h"
#include "Plane.h"
#include "Triangle.h"
#include "Photon.h"

#define PHOTONMUM 1000
#define PHOTONUSE 5
#define PHOTONDIST 0.005

#define PI 3.1415926
#define BOUNCE 3

using namespace std;

Photon photonMap[PHOTONMUM*BOUNCE*BOUNCE*BOUNCE*BOUNCE];
int storedPhotonMum = 0;

struct RGBType{
	double r;
	double g;
	double b;
};

void saveBmp(const char *filename, int w, int h, int dpi, RGBType *data){
	FILE *f;
	int k = w*h;
	int s = 4*k;
	int filesize = 54 + s;

	double factor = 39.375;
	int m = static_cast<int>(factor);

	int ppm = dpi*m;

	unsigned char bmpFileHeader[14] = {'B','M',0,0,0,0,0,0,0,0,54,0,0,0};
	unsigned char bmpInfoHeader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,24,0};

	bmpFileHeader[2] = (unsigned char)(filesize);
	bmpFileHeader[3] = (unsigned char)(filesize>>8);
	bmpFileHeader[4] = (unsigned char)(filesize>>16);
	bmpFileHeader[5] = (unsigned char)(filesize>>24);

	bmpInfoHeader[4] = (unsigned char)(w);
	bmpInfoHeader[5] = (unsigned char)(w>>8);
	bmpInfoHeader[6] = (unsigned char)(w>>16);
	bmpInfoHeader[7] = (unsigned char)(w>>24);
	
	bmpInfoHeader[8]  = (unsigned char)(h);
	bmpInfoHeader[9]  = (unsigned char)(h>>8);
	bmpInfoHeader[10] = (unsigned char)(h>>16);
	bmpInfoHeader[11] = (unsigned char)(h>>24);

	bmpInfoHeader[21] = (unsigned char)(s);
	bmpInfoHeader[22] = (unsigned char)(s>>8);
	bmpInfoHeader[23] = (unsigned char)(s>>16);
	bmpInfoHeader[24] = (unsigned char)(s>>24);

	bmpInfoHeader[25] = (unsigned char)(ppm);
	bmpInfoHeader[26] = (unsigned char)(ppm>>8);
	bmpInfoHeader[27] = (unsigned char)(ppm>>16);
	bmpInfoHeader[28] = (unsigned char)(ppm>>24);

	bmpInfoHeader[29] = (unsigned char)(ppm);
	bmpInfoHeader[30] = (unsigned char)(ppm>>8);
	bmpInfoHeader[31] = (unsigned char)(ppm>>16);
	bmpInfoHeader[32] = (unsigned char)(ppm>>24);

	f = fopen(filename,"wb");

	fwrite(bmpFileHeader,1,14,f);
	fwrite(bmpInfoHeader,1,40,f);

	for (int i = 0; i < k; i++){
		RGBType rgb = data[i];

		double red = data[i].r*255;
		double green = data[i].g*255;
		double blue = data[i].b*255;

		unsigned char color[3] = {(unsigned char)floor(blue), (unsigned char)floor(green), (unsigned char)floor(red)};

		fwrite(color,1,3,f);
	}

	fclose(f);
}

int winningObjectIndex(vector<double> object_intersections){
	//return the index of winning intersection
	int index_of_minimum_value;

	if (object_intersections.size() == 0){
		return -1;
	}
	else if (object_intersections.size() == 1){
		if (object_intersections.at(0) > 0){
			return 0;
		}
		else{
			return -1;
		}
	}
	else{
		double max = 0;
		for (int i =0; i <object_intersections.size(); i++){
			if (max < object_intersections.at(i))
				max = object_intersections.at(i);
		}

		if (max > 0){
			for (int index = 0; index < object_intersections.size(); index++){
				if (object_intersections.at(index) > 0 && object_intersections.at(index) <= max){
					max = object_intersections.at(index);
					index_of_minimum_value = index;
				}
			}
			return index_of_minimum_value;
		}
		else{
			return -1;
		}
	}
}

Color storePhoton(Vect intersection_position, Vect intersecting_ray_direction, vector<Object*> scene_objects, int index_of_winning_object, vector<Source*> light_sources, double accuracy, double ambientLight, Color lightColor, int bounce){
	Color winning_object_color = scene_objects.at(index_of_winning_object)->getColor();
	
	Vect winning_object_normal = scene_objects.at(index_of_winning_object)->getNormalAt(intersection_position);

	//Color final_color = winning_object_color/*.colorScalar(ambientLight)*/;

	Color final_color;
	final_color.setColorRed(min(winning_object_color.getColorRed(), lightColor.getColorRed()));
	final_color.setColorGreen(min(winning_object_color.getColorGreen(), lightColor.getColorGreen()));
	final_color.setColorBlue(min(winning_object_color.getColorBlue(), lightColor.getColorBlue()));

	

	/*
	for (int light_index = 0; light_index < light_sources.size(); light_index++){
		Vect light_direction = light_sources.at(light_index)->getLightPosition().vectAdd(intersection_position.negtive()).normalize();
				
		float cosine_angle = winning_object_normal.dotProduct(light_direction);

		if (cosine_angle > 0){
			//test shadow
			bool shadowed = false;

			Vect distance_to_light = light_sources.at(light_index)->getLightPosition().vectAdd(intersection_position.negtive()); // no normalize
			float distance_to_light_magnitude = distance_to_light.magnitude();

			Ray shadow_ray(intersection_position, light_sources.at(light_index)->getLightPosition().vectAdd(intersection_position.negtive()).normalize());

			vector<double> secondary_intersections;
			for (int object_index = 0; object_index < scene_objects.size() && shadowed == false; object_index++){
				secondary_intersections.push_back(scene_objects.at(object_index)->findIntersection(shadow_ray));
			}

			for (int c = 0; c < secondary_intersections.size(); c++){
				if (secondary_intersections.at(c) > accuracy){
					if (secondary_intersections.at(c) <= distance_to_light_magnitude){
						shadowed = true;
					}
					break;
				}
			}

			if (shadowed == false){
				final_color = final_color.colorAdd(winning_object_color.colorMultiply(light_sources.at(light_index)->getLightColor()).colorScalar(cosine_angle));

				if (winning_object_color.getColorSpecial() > 0 && winning_object_color.getColorSpecial() <= 1){
					double dot1 = winning_object_normal.dotProduct(intersecting_ray_direction.negtive()); // cos of objNormal and rayDir
					Vect scalar1 = winning_object_normal.vectMult(dot1); // 
					Vect add1 = scalar1.vectAdd(intersecting_ray_direction);
					Vect scalar2 = add1.vectMult(2);
					Vect add2 = intersecting_ray_direction.negtive().vectAdd(scalar2);
					Vect reflection_dir = add2.normalize();

					double specular = reflection_dir.dotProduct(light_direction);
					if (specular > 0){
						specular = pow(specular, 10);
						final_color = final_color.colorAdd(light_sources.at(light_index)->getLightColor().colorScalar(specular*winning_object_color.getColorSpecial()));
					}
 				}
			}
		}
	}
	*/

	char phi = 255 * (atan2(intersecting_ray_direction.getVectX(), intersecting_ray_direction.getVectY())+ PI) / (2*PI);
	char theta = 255 * acos(intersecting_ray_direction.getVectX()) / PI;

	//photonMap[storedPhotonMum++] = Photon(intersection_position, final_color.clip(), phi, theta, intersecting_ray_direction);
	photonMap[storedPhotonMum++] = Photon(intersection_position, lightColor, phi, theta, intersecting_ray_direction);
	photonMap[storedPhotonMum++].bounce = bounce;
	return final_color.clip();
}

Color getColorAt(Vect intersection_position, Vect intersecting_ray_direction, vector<Object*> scene_objects, int index_of_winning_object, vector<Source*> light_sources, double accuracy, double ambientLight){
	Color winning_object_color = scene_objects.at(index_of_winning_object)->getColor();
	Vect winning_object_normal = scene_objects.at(index_of_winning_object)->getNormalAt(intersection_position);

	Color final_color = winning_object_color.colorScalar(ambientLight);


	if (winning_object_color.getColorSpecial() > 0 && winning_object_color.getColorSpecial() <= 1) {
		//reflection from object with specular intensity
		
		double dot1 = winning_object_normal.dotProduct(intersecting_ray_direction.negtive());
		Vect scalar1 = winning_object_normal.vectMult(dot1);
		Vect add1 = scalar1.vectAdd(intersecting_ray_direction);
		Vect scalar2 = add1.vectMult(2);
		Vect add2 = intersecting_ray_direction.negtive().vectAdd(scalar2);
		Vect reflection_direction = add2.normalize();

		Ray reflection_ray(intersection_position, reflection_direction);

		//determine what the ray intersects first
		vector<double> reflection_intersections;

		for (int reflection_index = 0; reflection_index < scene_objects.size(); reflection_index++) {
			reflection_intersections.push_back(scene_objects.at(reflection_index)->findIntersection(reflection_ray));
		}

		int index_of_winning_object_reflection = winningObjectIndex(reflection_intersections);

		if (index_of_winning_object_reflection != -1) {
			//no miss
			if (reflection_intersections.at(index_of_winning_object_reflection) > accuracy) {
				Vect reflection_intersection_position = intersection_position.vectAdd(reflection_direction.vectMult(reflection_intersections.at(index_of_winning_object_reflection)));
				Vect reflection_intersection_direction = reflection_direction;

				Color reflection_intersection_color = getColorAt(reflection_intersection_position, reflection_intersection_direction, scene_objects, index_of_winning_object_reflection, light_sources, accuracy, ambientLight);

				final_color = final_color.colorAdd(reflection_intersection_color.colorScalar(winning_object_color.getColorSpecial()));
			}
		}
	}

	else {
		int pn = 0;
		Photon tmpPhoton(Vect(100,100,100), Color(0,0,0,0), 0, 0, Vect(0,0,0));

		Photon removedPhoton[PHOTONUSE];
		int removedIndex[PHOTONUSE];
		int actuallyPhotonUse = 0;
		
		while(pn < PHOTONUSE) {
			
			float mindist = 9999;
			int minidx = -1;
	
			for (int i = 0; i < storedPhotonMum; i++) {
				if (photonMap[i].used == 1) continue;
				Vect pos = photonMap[i].position;
				float dist = pow((pos.getVectX()-intersection_position.getVectX()), 2) + 
							pow((pos.getVectY()-intersection_position.getVectY()), 2) + 
							pow((pos.getVectZ()-intersection_position.getVectZ()), 2);
				if (dist < mindist && dist < PHOTONDIST) {
					minidx = i;
					mindist = dist;
				}	
			}

			if (minidx == -1) {
				actuallyPhotonUse = pn;
				break;
			}

			photonMap[minidx].used = 1;
			removedPhoton[pn] = photonMap[minidx];
			removedIndex[pn] = minidx;
	
	
	/************************************************************************/
	/* estimate ray wtf?                                                    */
	/************************************************************************/
			Vect light_direction = photonMap[minidx].dir.negtive();
					
			float cosine_angle = winning_object_normal.dotProduct(light_direction);
	
			if (cosine_angle > 0){
				//test shadow
				bool shadowed = false;
	
				//Vect distance_to_light = light_sources.at(0)->getLightPosition().vectAdd(intersection_position.negtive()); // no normalize
				//float distance_to_light_magnitude = distance_to_light.magnitude();
				/*
				Ray shadow_ray(intersection_position, light_sources.at(0)->getLightPosition().vectAdd(intersection_position.negtive()).normalize());
	
				vector<double> secondary_intersections;
				for (int object_index = 0; object_index < scene_objects.size() && shadowed == false; object_index++){
					secondary_intersections.push_back(scene_objects.at(object_index)->findIntersection(shadow_ray));
				}
				*/
	
				final_color = final_color.colorAdd(winning_object_color.colorMultiply(photonMap[minidx].power).colorScalar(cosine_angle));
	
				/*
				for (int c = 0; c < secondary_intersections.size(); c++){
					if (secondary_intersections.at(c) > accuracy){
						if (secondary_intersections.at(c) <= distance_to_light_magnitude){
							shadowed = true;
						}
						break;
					}
				}
	
				if (shadowed == false){
					final_color = final_color.colorAdd(winning_object_color.colorMultiply(light_sources.at(0)->getLightColor()).colorScalar(cosine_angle));
	
					if (winning_object_color.getColorSpecial() > 0 && winning_object_color.getColorSpecial() <= 1){
						double dot1 = winning_object_normal.dotProduct(intersecting_ray_direction.negtive()); // cos of objNormal and rayDir
						Vect scalar1 = winning_object_normal.vectMult(dot1); // 
						Vect add1 = scalar1.vectAdd(intersecting_ray_direction);
						Vect scalar2 = add1.vectMult(2);
						Vect add2 = intersecting_ray_direction.negtive().vectAdd(scalar2);
						Vect reflection_dir = add2.normalize();
	
						double specular = reflection_dir.dotProduct(light_direction);
						if (specular > 0){
							specular = pow(specular, 10);
							final_color = final_color.colorAdd(light_sources.at(0)->getLightColor().colorScalar(specular*winning_object_color.getColorSpecial()));
						}
						
	// 					Vect R =  winning_object_normal.vectMult(winning_object_normal.dotProduct(light_direction) * 2).vectAdd(light_direction.negtive());
	// 
	// 					double specular = winning_object_color.getColorSpecial() * pow( intersecting_ray_direction.negtive().dotProduct(R), 10 );
	// 
	// 					if (specular > 0){
	// 						//specular = pow(specular, 10);
	// 						final_color = final_color.colorAdd(light_sources.at(light_index)->getLightColor().colorScalar(specular));
	// 					}
	 				}
				}*/
			}
	
			//final_color = final_color.colorAdd(photonMap[minidx].power);
			photonMap[minidx] = tmpPhoton;
			pn++;
		}

		if (pn == PHOTONUSE) {
			actuallyPhotonUse = pn;
		}
		
		if (actuallyPhotonUse != 0) {
			final_color = final_color.colorScalar(1.0/(actuallyPhotonUse));
			//final_color = final_color.colorScalar(1.0/(4*PI));
		}
		
	
		for (int haha = 0; haha < actuallyPhotonUse; haha++) {
			photonMap[removedIndex[haha]] = removedPhoton[haha];
			photonMap[removedIndex[haha]].used = 0;
		}
		
		//for (int i = 0; i < PHOTONMUM; i++) photonMap[i].used = 0;
	
	
		/*
		for (int i = 0; i < storedPhotonMum; i++) {
			//if (photonMap[i].used == 1) continue;
			Vect pos = photonMap[i].position;
			float dist = pow((pos.getVectX()-intersection_position.getVectX()), 2) + 
						pow((pos.getVectY()-intersection_position.getVectY()), 2) + 
						pow((pos.getVectZ()-intersection_position.getVectZ()), 2);
			if (dist < 0.00001) {
				final_color = final_color.colorAdd(photonMap[i].power);
				//break;
			}	
		}
		*/
	
	
		/*
		if (winning_object_color.getColorSpecial() == 2){
			//checkerboard pattern
	
			int square = (int)floor(intersection_position.getVectX()) + (int)floor(intersection_position.getVectZ());  //dafuq?
	
			if ((square % 2) == 0) {
				winning_object_color.setColorRed(0);
				winning_object_color.setColorGreen(0);
				winning_object_color.setColorBlue(0);
			}
			else {
				winning_object_color.setColorRed(1);
				winning_object_color.setColorGreen(1);
				winning_object_color.setColorBlue(1);
			}
		}
		
		Color final_color = winning_object_color.colorScalar(ambientLight);
	
		if (winning_object_color.getColorSpecial() > 0 && winning_object_color.getColorSpecial() <= 1) {
			//reflection from object with specular intensity
			
			double dot1 = winning_object_normal.dotProduct(intersecting_ray_direction.negtive());
			Vect scalar1 = winning_object_normal.vectMult(dot1);
			Vect add1 = scalar1.vectAdd(intersecting_ray_direction);
			Vect scalar2 = add1.vectMult(2);
			Vect add2 = intersecting_ray_direction.negtive().vectAdd(scalar2);
			Vect reflection_direction = add2.normalize();
	
			Ray reflection_ray(intersection_position, reflection_direction);
	
			//determine what the ray intersects first
			vector<double> reflection_intersections;
	
			for (int reflection_index = 0; reflection_index < scene_objects.size(); reflection_index++) {
				reflection_intersections.push_back(scene_objects.at(reflection_index)->findIntersection(reflection_ray));
			}
	
			int index_of_winning_object_reflection = winningObjectIndex(reflection_intersections);
	
			if (index_of_winning_object_reflection != -1) {
				//no miss
				if (reflection_intersections.at(index_of_winning_object_reflection) > accuracy) {
					Vect reflection_intersection_position = intersection_position.vectAdd(reflection_direction.vectMult(reflection_intersections.at(index_of_winning_object_reflection)));
					Vect reflection_intersection_direction = reflection_direction;
	
					Color reflection_intersection_color = getColorAt(reflection_intersection_position, reflection_intersection_direction, scene_objects, index_of_winning_object_reflection, light_sources, accuracy, ambientLight);
	
					final_color = final_color.colorAdd(reflection_intersection_color.colorScalar(winning_object_color.getColorSpecial()));
				}
			}
	
		}
		
		for (int light_index = 0; light_index < light_sources.size(); light_index++){
			Vect light_direction = light_sources.at(light_index)->getLightPosition().vectAdd(intersection_position.negtive()).normalize();
					
			float cosine_angle = winning_object_normal.dotProduct(light_direction);
	
			if (cosine_angle > 0){
				//test shadow
				bool shadowed = false;
	
				Vect distance_to_light = light_sources.at(light_index)->getLightPosition().vectAdd(intersection_position.negtive()); // no normalize
				float distance_to_light_magnitude = distance_to_light.magnitude();
	
				Ray shadow_ray(intersection_position, light_sources.at(light_index)->getLightPosition().vectAdd(intersection_position.negtive()).normalize());
	
				vector<double> secondary_intersections;
				for (int object_index = 0; object_index < scene_objects.size() && shadowed == false; object_index++){
					secondary_intersections.push_back(scene_objects.at(object_index)->findIntersection(shadow_ray));
				}
	
				for (int c = 0; c < secondary_intersections.size(); c++){
					if (secondary_intersections.at(c) > accuracy){
						if (secondary_intersections.at(c) <= distance_to_light_magnitude){
							shadowed = true;
						}
						break;
					}
				}
	
				if (shadowed == false){
					final_color = final_color.colorAdd(winning_object_color.colorMultiply(light_sources.at(light_index)->getLightColor()).colorScalar(cosine_angle));
	
					if (winning_object_color.getColorSpecial() > 0 && winning_object_color.getColorSpecial() <= 1){
						double dot1 = winning_object_normal.dotProduct(intersecting_ray_direction.negtive()); // cos of objNormal and rayDir
						Vect scalar1 = winning_object_normal.vectMult(dot1); // 
						Vect add1 = scalar1.vectAdd(intersecting_ray_direction);
						Vect scalar2 = add1.vectMult(2);
						Vect add2 = intersecting_ray_direction.negtive().vectAdd(scalar2);
						Vect reflection_dir = add2.normalize();
	
						double specular = reflection_dir.dotProduct(light_direction);
						if (specular > 0){
							specular = pow(specular, 10);
							final_color = final_color.colorAdd(light_sources.at(light_index)->getLightColor().colorScalar(specular*winning_object_color.getColorSpecial()));
						}
						
	// 					Vect R =  winning_object_normal.vectMult(winning_object_normal.dotProduct(light_direction) * 2).vectAdd(light_direction.negtive());
	// 
	// 					double specular = winning_object_color.getColorSpecial() * pow( intersecting_ray_direction.negtive().dotProduct(R), 10 );
	// 
	// 					if (specular > 0){
	// 						//specular = pow(specular, 10);
	// 						final_color = final_color.colorAdd(light_sources.at(light_index)->getLightColor().colorScalar(specular));
	// 					}
	 				}
				}
			}
		}
		*/		
	}
	return final_color.clip();
}

int thisone;
vector<Object*> scene_objects;

void makeCube (Vect corner1, Vect corner2, Color color) {
	double c1x = corner1.getVectX();
	double c1y = corner1.getVectY();
	double c1z = corner1.getVectZ();

	double c2x = corner2.getVectX();
	double c2y = corner2.getVectY();
	double c2z = corner2.getVectZ();

	Vect A (c2x, c1y, c1z);
	Vect B (c2x, c1y, c2z);
	Vect C (c1x, c1y, c2z);
	
	Vect D (c2x, c2y, c1z);
	Vect E (c1x, c2y, c1z);
	Vect F (c1x, c2y, c2z);

	//left side
	scene_objects.push_back(new Triangle(D, A, corner1, color));
	scene_objects.push_back(new Triangle(corner1, E, D, color));
	//far side
	scene_objects.push_back(new Triangle(corner2, B, A, color));
	scene_objects.push_back(new Triangle(A, D, corner2, color));
	//right side
	scene_objects.push_back(new Triangle(F, C, B, color));
	scene_objects.push_back(new Triangle(B, corner2, F, color));
	//front side
	scene_objects.push_back(new Triangle(E, corner1, C, color));
	scene_objects.push_back(new Triangle(C, F, E, color));
	//top
	scene_objects.push_back(new Triangle(D, E, F, color));
	scene_objects.push_back(new Triangle(F, corner2, D, color));
	//bottom
	scene_objects.push_back(new Triangle(corner1, A, B, color));
	scene_objects.push_back(new Triangle(B, C, corner1, color));
}

void makeCornellBox (Vect corner1, Vect corner2) {
	double c1x = corner1.getVectX();
	double c1y = corner1.getVectY();
	double c1z = corner1.getVectZ();

	double c2x = corner2.getVectX();
	double c2y = corner2.getVectY();
	double c2z = corner2.getVectZ();

	Vect A (c2x, c1y, c1z);
	Vect B (c2x, c1y, c2z);
	Vect C (c1x, c1y, c2z);
	
	Vect D (c2x, c2y, c1z);
	Vect E (c1x, c2y, c1z);
	Vect F (c1x, c2y, c2z);


	Color red(1, 0.25, 0.25, 0);
	Color green(0.25, 1, 0.25, 0);
	Color white(1.0, 1.0, 1.0, 0);


	//left side
	scene_objects.push_back(new Triangle(D, A, corner1, green));
	scene_objects.push_back(new Triangle(corner1, E, D, green));
	//far side
	scene_objects.push_back(new Triangle(corner2, B, A, white));
	scene_objects.push_back(new Triangle(A, D, corner2, white));
	//right side
	scene_objects.push_back(new Triangle(F, C, B, red));
	scene_objects.push_back(new Triangle(B, corner2, F, red));
	//front side
	//scene_objects.push_back(new Triangle(E, corner1, C, white));
	//scene_objects.push_back(new Triangle(C, F, E, white));
	//top
	scene_objects.push_back(new Triangle(D, E, F, white));
	scene_objects.push_back(new Triangle(F, corner2, D, white));
	//bottom
	scene_objects.push_back(new Triangle(corner1, A, B, white));
	scene_objects.push_back(new Triangle(B, C, corner1, white));
}



int main (int argc, char* argv[]){
	cout<<"rendering..."<<endl <<"Preparing Scene..."<<endl;

	clock_t t1, t2, tPrep, tTracing;
	t1 = clock();

	int dpi = 72;
	int width = 480;
	int height = 480;
	int n = width * height;

	int aadepth = 1;
	double aathreshold = .1;
	double aspectRatio = (double)width/(double)height;
	double ambientLight = 0.2;
	double accuracy = 0.000001;

	RGBType *pixels = new RGBType[n];

	Vect O (0,0,0);
	Vect X (1,0,0);
	Vect Y (0,1,0);
	Vect Z (0,0,1);

	Vect new_sphere_pos (0.3, -0.7, -0.4);
	Vect new_sphere_pos2 (0.3, -0.7, 0.4);

	Vect camPos(2.8,0,0);

	Vect look_at(0,0,0);
	Vect diff_btw(camPos.getVectX() - look_at.getVectX(), camPos.getVectY() - look_at.getVectY(), camPos.getVectZ() - look_at.getVectZ());

	Vect camDir = diff_btw.negtive().normalize();
	Vect camRight = Y.crossProduct(camDir).normalize();
	Vect camDown = camRight.crossProduct(camDir);

	Camera scene_cam (camPos, camDir, camRight, camDown);

	Color white_light (1.0,1.0,1.0,0.0);
	//Color pretty_green (0.5,1.0,0.5,0.3);
	Color pretty_green (1.0,1.0,1.0,0.9);
	Color maroon(0.5,0.25,0.25,2);
	Color pretty_maroon(0.5,0.25,0.25,0.6);
	Color gray(0.5,0.5,0.5,0);
	Color black(0.0,0.0,0.0,0);
	Color orange(0.94, 0.75, 0.31, 0);

	Vect light_position(0,0.9,0);
	Light scene_light (light_position, white_light);
	vector<Source*> light_sources;
	light_sources.push_back(dynamic_cast<Source*>(&scene_light));

	Sphere scene_sphere (new_sphere_pos, 0.3 ,pretty_green);
	Sphere scene_sphere2 (new_sphere_pos2, 0.3 ,orange);
	Plane scene_plane(Y, -1, maroon);
	Triangle scene_triangle (Vect(3,0,0), Vect(0,3,0), Vect(0,0,3), orange);

	scene_objects.push_back(dynamic_cast<Object*>(&scene_sphere));
	//scene_objects.push_back(dynamic_cast<Object*>(&scene_plane));
	scene_objects.push_back(dynamic_cast<Object*>(&scene_sphere2));
	//scene_objects.push_back(dynamic_cast<Object*>(&scene_triangle));

	makeCornellBox(Vect (1,1,1), Vect (-1,-1,-1));
	//makeCube(Vect (0.5,0,0.5), Vect (-0.5,-1,-0.5), pretty_green);

	int thisone, aa_index;
	double xamnt, yamnt;  //dafuq?
	double tempRed, tempGreen, tempBlue;

	tPrep = clock();
	float diffPrep = ((float)tPrep - (float)t1)/CLOCKS_PER_SEC;
	cout<< diffPrep<<"seconds"<<endl;

	cout<<"start emit photons..."<<endl;


	int pn = 0;
	float x, y, z;

	srand(0);
	
	while (pn < PHOTONMUM) {
		do {
			x = (rand() % 200)/100.0f - 1;
			y = (rand() % 200)/100.0f - 1;
			z = (rand() % 200)/100.0f - 1;
		} while (x*x+y*y+z*z > 1);

		Vect photon_ray_direction(x,y,z);
		photon_ray_direction = photon_ray_direction.normalize();

		Ray photon_ray (light_position, photon_ray_direction);

		Color lightColor = white_light;

		for (int bounce = 0; bounce < BOUNCE; bounce++) {
			vector<double> intersections;

			for (int index = 0; index < scene_objects.size(); index++){
				intersections.push_back(scene_objects.at(index)->findIntersection(photon_ray));
			}

			int index_of_winning_object = winningObjectIndex(intersections);

			//cout << index_of_winning_object;
			if (index_of_winning_object == -1) {
				if (bounce == 0) pn--;
				break;
			}
		
			if (intersections.at(index_of_winning_object) > accuracy){
				Vect intersection_position = photon_ray.getRayOrigin().vectAdd(photon_ray_direction.vectMult(intersections.at(index_of_winning_object)));
				Vect intersecting_ray_direction = photon_ray_direction;
				
				//Color intersection_color = getColorAt(intersection_position, intersecting_ray_direction, scene_objects, index_of_winning_object, light_sources, accuracy, ambientLight);
				

				if (scene_objects.at(index_of_winning_object)->getColor().getColorSpecial() == 0) {
					lightColor = storePhoton(intersection_position, intersecting_ray_direction, scene_objects, index_of_winning_object, light_sources, accuracy, ambientLight, lightColor, bounce);	
				}
				lightColor = lightColor.colorScalar(0.8);
				//srand(intersection_position.getVectX()*100);
				//int roll = rand() % 100;
				//if (roll < 40) {
				//	break;
				//}
				//else {
				Vect winning_object_normal = scene_objects.at(index_of_winning_object)->getNormalAt(intersection_position);
				double dot1 = winning_object_normal.dotProduct(intersecting_ray_direction.negtive()); // N*L
				Vect scalar1 = winning_object_normal.vectMult(dot1); // (N*L)*N
				Vect add1 = scalar1.vectAdd(intersecting_ray_direction);
				Vect scalar2 = add1.vectMult(2);
				Vect add2 = intersecting_ray_direction.negtive().vectAdd(scalar2);
				Vect reflection_dir = add2.normalize();
				//Vect NLN2 = scalar1.vectMult(2);
				//Vect reflection_dir = NLN2.vectAdd(intersecting_ray_direction);

				photon_ray = Ray(intersection_position, reflection_dir);
				photon_ray_direction = reflection_dir;
					//srand(0);
				//}		
			}
		}
		pn++;
	}

	cout<<"start ray tracing..."<<endl;

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++){
			thisone = y*width + x;

			//start with black pix
			double* tempRed = new double[aadepth*aadepth];
			double* tempGreen = new double[aadepth*aadepth];
			double* tempBlue = new double[aadepth*aadepth];

			for (int aax = 0; aax < aadepth; aax++) {
				for (int aay = 0; aay < aadepth; aay++) {

					aa_index = aay*aadepth+ + aax;

					srand(time(0));

					if (aadepth == 1) {
						// no anti aliasing
						if (width > height) {
							// img is wider than tall
							xamnt = ((x+0.5)/width)*aspectRatio - (((width-height)/(double)height)/2);
							yamnt = ((height - y) + 0.5)/height;
						}
						else if (height > width) {
							xamnt = (x + 0.5)/width;
							yamnt = (((height - y) + 0.5)/height)/aspectRatio - (((height - width)/(double)width)/2);
						}
						else{
							//square image
							xamnt = (x + 0.5)/width;
							yamnt = ((height - y) + 0.5)/height;
						}
					}

					else {
						// AA
						if (width > height) {
							// img is wider than tall
							xamnt = ((x+(double)aax/((double)aadepth -1))/width)*aspectRatio - (((width-height)/(double)height)/2);
							yamnt = ((height - y) + (double)aax/((double)aadepth -1))/height;
						}
						else if (height > width) {
							xamnt = (x + (double)aax/((double)aadepth -1))/width;
							yamnt = (((height - y) + (double)aax/((double)aadepth -1))/height)/aspectRatio - (((height - width)/(double)width)/2);
						}
						else{
							//square image
							xamnt = (x + (double)aax/((double)aadepth -1))/width;
							yamnt = ((height - y) + (double)aax/((double)aadepth -1))/height;
						}
					}

					Vect cam_ray_origin = scene_cam.getCameraPosition();
					Vect cam_ray_direction = camDir.vectAdd(camRight.vectMult(xamnt - 0.5).vectAdd(camDown.vectMult(yamnt - 0.5))).normalize();

					Ray cam_ray (cam_ray_origin, cam_ray_direction);

					vector<double> intersections;

					for (int index = 0; index < scene_objects.size(); index++){
						intersections.push_back(scene_objects.at(index)->findIntersection(cam_ray));
					}

					int index_of_winning_object = winningObjectIndex(intersections);

					//cout << index_of_winning_object;

					if (index_of_winning_object == -1){
						tempRed[aa_index] = 0;
						tempGreen[aa_index] = 0;
						tempBlue[aa_index] = 0;
					}
					else{
						if (intersections.at(index_of_winning_object) > accuracy){
							Vect intersection_position = cam_ray.getRayOrigin().vectAdd(cam_ray_direction.vectMult(intersections.at(index_of_winning_object)));
							Vect intersecting_ray_direction = cam_ray_direction;
							
							Color intersection_color = getColorAt(intersection_position, intersecting_ray_direction, scene_objects, index_of_winning_object, light_sources, accuracy, ambientLight);
							
							tempRed[aa_index]   = intersection_color.getColorRed();
							tempGreen[aa_index] = intersection_color.getColorGreen();
							tempBlue[aa_index]  = intersection_color.getColorBlue();
						}	
					}
				}
			//delete tempRed, tempGreen, tempBlue;
			}
			// return color

			double totalRed = 0;
			double totalGreen = 0;
			double totalBlue = 0;

			for (int iRed = 0; iRed < aadepth*aadepth; iRed++) {
				totalRed += tempRed[iRed];
			}
			for (int iGreen = 0; iGreen < aadepth*aadepth; iGreen++) {
				totalGreen += tempGreen[iGreen];
			}
			for (int iBlue = 0; iBlue < aadepth*aadepth; iBlue++) {
				totalBlue += tempBlue[iBlue];
			}

			double avgRed = totalRed/(aadepth*aadepth);
			double avgGreen = totalGreen/(aadepth*aadepth);
			double avgBlue = totalBlue/(aadepth*aadepth);

			pixels[thisone].r = avgRed;
			pixels[thisone].g = avgGreen;
			pixels[thisone].b = avgBlue;
		}
	}

	tTracing = clock();
	float diffTracing = ((float)tTracing - (float)tPrep)/CLOCKS_PER_SEC;
	cout<< diffTracing<<"seconds"<<endl << "saving file"<<endl;

	saveBmp("scene.bmp", width, height, dpi, pixels);
	cout<<"Finished"<<endl;

	delete pixels, tempRed, tempGreen, tempBlue;;

	t2 = clock();
	float diff = ((float)t2 - (float)t1)/CLOCKS_PER_SEC;

	cout<< diff<<"seconds"<<endl;

	system("pause");
	return 0;
}