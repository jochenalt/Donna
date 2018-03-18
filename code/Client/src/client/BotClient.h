/*
 * BotClient.h
 *
 *  Created on: Mar 14, 2018
 *      Author: JochenAlt
 */

#ifndef SRC_CLIENT_BOTCLIENT_H_
#define SRC_CLIENT_BOTCLIENT_H_

#include <client/HttpConnection.h>

#include <dance/Move.h>
#include <dance/Dancer.h>


class BotClient {
public:
	BotClient();
	virtual ~BotClient();

	// return singleton
	static BotClient& getInstance();

	// is webclient running?
	bool isActive() { return isWebClientActive; };

	// setup rest connection to server
	void setup(string host, int port);

	// fetch current state of the bot, used to continously get the current pose
	void getStatus();

	// switch between automatic or manual move change
	void setMoveMode(Dancer::SequenceModeType moveMode);

	// set the current move
	void setMove(Move::MoveType move);

	// set the current amplitude of the move
	void setAmbition(float ambition);

	Pose getHeadPose() { return headPose; };
	Pose getBodyPose() { return bodyPose; };
	double getAmbition() { return ambition; };
	Move::MoveType getMove() { return move; };

private:
	string get(string requestUrl, bool& ok);

	Pose bodyPose;
	Pose headPose;
	float ambition = 0;
	Move::MoveType move = Move::MoveType::NO_MOVE;
	bool isWebClientActive = false;

	HttpConnection conn;
};

#endif /* SRC_CLIENT_BOTCLIENT_H_ */