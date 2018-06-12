import matplotlib.pyplot as plt
import math
import random

#Need to clean up unused variables.
sensing_radius = 100
angle = 0
artifact = [0,0]
state = [0,0]
step = 4
SIM_STEP = -1
# aperture = [[0, 100],[-86.6,50],[0,0],[0, 100],[86.6,50],[0,0]]
# aperture = [[0, 100],[-86.6,50],[0,2],[0, 100],[86.6,50],[0,0]]
# aperture = [[0, 100],[-86.6,50],[0,0],[86.6,50],[0,100]]
aperture = [[-86.6,50], [0, 100],[86.6,50],[0,0]]
# aperture = [[50, 0],[0, 50],[-50,0],[0, 0]]
# aperture = [[0,50],[50, 50],[0, 100],[-50,50],[0, 50],[0,0]]
# aperture = [[50,86.6],[-50,86.6],[0, 0]]
# aperture = [[50,50],[-50,50],[0, 0]]
# aperture = [[0,25],[50,75],[-50,75],[0, 25],[0,0]]
# aperture = [[50,50],[0,100],[-50,50],[0, 0]]
color = (0,0,0)
pathx = []
pathy = []
distance = []

#distance between two R2 points
def dist(p1, p2):
	return math.sqrt( math.pow(p2[0]-p1[0],2) + math.pow(p2[1]-p1[1],2) )

#slope made by a line p1->p2 with the +X axis
def slope(p1, p2):
	return math.atan2((p2[1]-p1[1]), (p2[0]-p1[0]))

#interpolate distance along the line segment start->target
def advance(start, target, distance):
	point = [0,0]

	point[0] = start[0] + ( (distance/dist(start, target)) * (target[0] - start[0]) )
	point[1] = start[1] + ( (distance/dist(start, target)) * (target[1] - start[1]) )
	return point 

#color space heatmap
def advance3(start, target, distance):
	point = [0,0,0]

	point[0] = (start[0] + ( (distance/dist(start, target)) * 3 * (target[0] - start[0]) ))%1
	point[1] = (start[1] + ( (distance/dist(start, target)) * 1 * (target[1] - start[1]) ))%1
	point[2] = (start[2] + ( (distance/dist(start, target)) * 2 * (target[2] - start[2]) ))%1
	return point 

#sample artifacts either using random sampling or uniformly along the perimeter of the sensing semi-circle 
def sample_artifact(counter = -1, limit = -1):
	global angle, artifact
	if counter == -1 or limit == -1:
		angle = random.uniform(0,math.pi)
	else:
		angle = (counter/float(limit))* math.pi
	
		
	artifact =  (sensing_radius*math.cos(angle),sensing_radius*math.sin(angle))
	# print counter, limit
	# print "Angle: ", angle
	# print "At ", artifact

#sample a point in R2 space at random, within the sensing radius from origin
def sample_state():
	point = [1000,1000]
	while ((point[0]*point[0])  +  (point[1]*point[1]) > (sensing_radius*sensing_radius)):
		point = (random.uniform(-sensing_radius, sensing_radius), random.uniform(0, sensing_radius))
	return point

#plotting helper function
def plot(path, distance):
	plt.figure(1)
	plt.scatter(path[0],path[1], c='r')
	plt.draw()
	plt.figure(2)
	if(distance>sensing_radius):
		color = 'r'
	else:
		color = 'y'
	plt.scatter(SIM_STEP, distance, c=color)
	plt.draw()

#init plot helper function
def initialize_plots():
	global SIM_STEP
	plt.ion()
	plt.figure(1)
	plt.axis([-(sensing_radius+10),(sensing_radius+10),0,(sensing_radius+10)])
	circle=plt.Circle((0,0),100,color='b',fill=False)
	plt.gcf().gca().add_artist(circle)
	plt.axes().set_aspect('equal')
	plt.show()


	plt.figure(2)
	plt.axis([0,200,0,200])
	plt.show()

	SIM_STEP = -1



#THE OMNIPOTENT FUNCTION OF AWESOMENESS. Poor structure. Mostly procedural.
def inflection_plots(NUM_ARTIFACTS=180, NUM_STATES=3000, NUM_DIRECTIONS = 180, all_at_once = False):
	global sensing_radius
	global angle
	global artifact
	global state
	global step
	global SIM_STEP
	global aperture
	global pathx
	global pathy
	global distance
	global color


	#Set up the plot
	if(all_at_once):
		plt.ion()
	plt.figure(3)
	plt.axis([-(sensing_radius+10),(sensing_radius+10),0,(sensing_radius+10)])
	circle=plt.Circle((0,0),100,color='b',fill=False)
	plt.gcf().gca().add_artist(circle)
	plt.axes().set_aspect('equal')
	if(all_at_once):
		plt.show()


	# Sample NUM_ARTIFACTS number of artifacts and scatter plot
	artifacts = []
	counter = 0
	while counter<NUM_ARTIFACTS:
		SIM_STEP = -1
		counter = counter + 1
		if counter%50 == 0:
			print(counter)
		sample_artifact(counter, NUM_ARTIFACTS)
		color = advance3([0,0,0],[1,1,1],((angle/math.pi)*math.sqrt(2)))
		color = (color[0],color[1],color[2])

		artifacts.append(artifact)

		plt.figure(3)
		plt.scatter(artifact[0], artifact[1], s=100, c=color)

		
		if(all_at_once):
			raw_input()

	counter = 0

	#****************************The variables that aggregate the field*******************************
	field = dict() #stores the number of inflections for every direction for every sampled state
	optimal_field = dict() #stores the maximum number of inflections and direction for every sampled state
	#*************************************************************************************************

	#Randomly sample NUM_STATES number of states and calculate optimal direction and number of inflections information
	while counter<NUM_STATES:
		counter = counter+1
		state = sample_state()
		field[state] = []
		plt.scatter(state[0], state[1], s=10, c='r')
		origin = [0,0]
		directions = 0

		#Sample uniformly over 2pi, NUM_DIRECTIONS number of directions
		while directions<NUM_DIRECTIONS:
			sampled_direction = (directions/float(NUM_DIRECTIONS))*2*math.pi
			directions = directions+1
			inflections = 0
			
			#For this state, check how many artifacts cause an inflection in the distance reading, given this direction
			for i, arf in enumerate(artifacts):
				slope_to_artifact = slope(state, arf)

				dist_to_arf = dist(state, arf)

				#Ignore artifact if beyond sensing radius			
				if dist_to_arf>sensing_radius:
					continue

				delta = sampled_direction - slope_to_artifact

				if(math.cos(delta) > 0):
					inflections = inflections+1

			#Store the number of inflections for this direction
			field[state].append([sampled_direction, inflections])
	
		#Sort all the directions by number of inflections
		field[state].sort(key=lambda x:x[1], reverse=True)
		arrow = 5
		small_arrow = 2
		max_field = field[state][0][1] #optimal direction at this state
		optimal_field[state] = [max_field]  #update the optimal field
		print(counter, " : ", state)


		for i,dirctn in enumerate(field[state]):
			arrow_end = [ state[0] + (arrow*math.cos(dirctn[0])) , state[1] + (arrow*math.sin(dirctn[0])) ]			
			small_arrow_end = [ state[0] + (small_arrow*math.cos(dirctn[0])) , state[1] + (small_arrow*math.sin(dirctn[0])) ]
			if dirctn[1]==max_field:
				optimal_field[state].append([[state[0], arrow_end[0]], [state[1], arrow_end[1]]])
			else:
				pass #Used to show other directions if necessary



	max_inflections = -1
	min_inflections = 1000000
	for key in optimal_field.keys():
		if optimal_field[key][0] > max_inflections:
			max_inflections = optimal_field[key][0]
		if optimal_field[key][0] < min_inflections:
			min_inflections = optimal_field[key][0]

	#Plot the field by coloring the directions by their nearness to the global optimal number of inflections
	print(max_inflections, min_inflections)
	for key in optimal_field.keys():
		for i, arrow in enumerate(optimal_field[key]):
			if i==0:
				continue
			color_ratio =  ((optimal_field[key][0] - min_inflections) * (optimal_field[key][0] - min_inflections)) / float(  (max_inflections - min_inflections) * (max_inflections - min_inflections)  )
			plt.plot(arrow[0], arrow[1], c=[color_ratio, 0.2, color_ratio], lw=1)
		plt.scatter(key[0], key[1], s=40, c=[0,0,1])





	if(not all_at_once):
		plt.show()




inflection_plots(180, 500, 180)




