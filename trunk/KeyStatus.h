/* KeyStatus.h
Michael Zahniser, 26 Oct 2013

Class representing which keys are currently held down.
*/

#ifndef KEY_STATUS_H_INCLUDED
#define KEY_STATUS_H_INCLUDED



class KeyStatus {
public:
	static const int THRUST = 1;
	static const int TURN = 2;
	static const int BACK = 4;
	static const int LAND = 8;
	static const int HYPERSPACE = 16;
	static const int TARGET_NEAR = 32;
	static const int PRIMARY = 64;
	
	static const int LEFT = -1;
	static const int RIGHT = -2;
	
public:
	KeyStatus();
	
	// Reset the key status (for example, because we're about to create a pop-up
	// window that may intercept the key up for any key that we have down).
	void Clear();
	// Set to the given status.
	void Set(int status, int turn = 0);
	
	// Update based on SDL's tracking of the key states.
	void Update();
	
	int Status() const;
	double Thrust() const;
	double Turn() const;
	
	
private:
	int status;
	int turn;
};



#endif
