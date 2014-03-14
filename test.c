/*
 * test.c
 *
 *  Created on: 10 Mar 2014
 *      Author: chris
 */

#include "NetComm.h"
#include "Server.h"
#include <stdio.h>

typedef struct pkt10u {
	packet_t packet;
	floorNo_t floor;
	playerNo_t player_number;
	pos_t xPos;
	pos_t yPos;
	vel_t xVel;
	vel_t yVel;
} PKT_POS_UPDATE_U;

typedef struct pkt08u {
	packet_t packet;
	int objectives_captured[MAX_OBJECTIVES];
	status_t game_status;
} PKT_GAME_STATUS_U;

packet_t input;
SOCKET s;
SOCKET u;
void getInput();
void getReply();
void getUDP();

playerNo_t myPlayerNumber = 0;
playerNo_t tagger = 0, taggee = 1;
floorNo_t myFloor = 0;
floorNo_t newFloor;
teamNo_t myTeam = 0;
pos_t myPosx = 0;
pos_t myPosy = 0;
status_t myReadyStatus = 0;
status_t myGameStatus = 0;
int running = 1;
int bye = 0;
vel_t myVelx = 0;
vel_t myVely = 0;

struct sockaddr_in server, client;

int main(int argc, char * argv[]) {

//connect to tcp port

	fd_set fdset;
	//int numLiveSockets;
	//SOCKET highSocket;

	server.sin_family = AF_INET;
	server.sin_port = htons(TCP_PORT);
	//server.sin_addr.s_addr =inet_addr(argv[1]);
	server.sin_addr.s_addr = inet_addr("192.168.0.49");

	client.sin_family = AF_INET;
	client.sin_port = htons(TCP_PORT);
	client.sin_addr.s_addr = htonl(INADDR_ANY);

	s = socket(AF_INET, SOCK_STREAM, 0);
	bind(s, (struct sockaddr*) &client, sizeof(client));
	connect(s, (struct sockaddr*) &server, sizeof(server));

	//Set up UDP
	server.sin_port = htons(UDP_PORT);
	client.sin_port = htons(UDP_PORT);
	if ((u = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
		perror("UDP Socket");
	}

	if (bind(u, (struct sockaddr*) &client, sizeof(client)) == -1) {
		perror("UDP bind");
	}

	while (running == 1) {
		FD_ZERO(&fdset);
		FD_SET(1, &fdset);
		FD_SET(s, &fdset);
		FD_SET(u, &fdset);

		select(((s > u) ? s : u) + 1, &fdset, NULL, NULL, NULL);

		if (FD_ISSET(1, &fdset)) {
			getInput();
		}

		if (FD_ISSET(s, &fdset)) {
			getReply();
		}

		if (FD_ISSET(u, &fdset)) {
			getUDP();
		}
	}
	return 0;
}

void getInput() {
	PKT_PLAYER_JOIN *bufPlayerJoin = malloc(sizeof(PKT_PLAYER_JOIN));
	PKT_POS_UPDATE_U *bufPlayerPos = (PKT_POS_UPDATE_U*) malloc(sizeof(PKT_POS_UPDATE_U));
	PKT_GAME_STATUS_U *bufGameStatus = (PKT_GAME_STATUS_U*) malloc(sizeof(PKT_GAME_STATUS_U));
	PKT_READY_STATUS *bufUpdatePlayer = malloc(sizeof(PKT_READY_STATUS));
	PKT_FLOOR_MOVE_REQUEST *bufPlayerFloorMove = malloc(sizeof(PKT_FLOOR_MOVE_REQUEST));
	PKT_TAGGING *bufTag = malloc(sizeof(PKT_TAGGING));

	char name[MAX_NAME] = "test me now!";
	char name2[MAX_NAME] = "Say name bitch!";
	fscanf(stdin, "%d", &input);
	int i;

	int varInt;
	double varDub;
	char varName[MAX_NAME];

	switch (input) {

	case 1:
		memcpy(bufPlayerJoin->client_player_name, name, MAX_NAME);

		printf("Sending packet 1:\n\tPlayer name: %s\n\n", bufPlayerJoin->client_player_name);

		send(s, &input, sizeof(packet_t), 0);
		send(s, bufPlayerJoin, sizeof(PKT_PLAYER_JOIN), 0);
		break;

	case 5:
		printf("Enter new team number: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myTeam = varInt;
		}

		printf("Enter new name: ");
		if (fscanf(stdin, "%s", varName) > 0) {
			memcpy(name2, varName, MAX_NAME);
		}

		printf("Enter Ready status: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myReadyStatus = varInt;
		}

		bufUpdatePlayer->player_number = myPlayerNumber;
		bufUpdatePlayer->ready_status = myReadyStatus;
		bufUpdatePlayer->team_number = myTeam;
		memcpy(bufUpdatePlayer->player_name, name2, MAX_NAME);

		printf("Send packet 5:\n\tPlayer Number: %d\n\tTeam number: %d\n\tPlayer name: %s\n\tReady status: %d\n\n",
				bufUpdatePlayer->player_number, bufUpdatePlayer->team_number, bufUpdatePlayer->player_name,
				bufUpdatePlayer->ready_status);

		send(s, &input, sizeof(packet_t), 0);
		send(s, bufUpdatePlayer, sizeof(PKT_READY_STATUS), 0);
		break;

	case 8:
		for (i = 0; i < MAX_OBJECTIVES; ++i) {
			bufGameStatus->objectives_captured[i] = 0;
		}

		printf("Enter new games status: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myGameStatus = varInt;
		}

		printf("Enter objective bit to flip: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			if (varInt >= 0 && varInt < MAX_OBJECTIVES) {
				if (bufGameStatus->objectives_captured[varInt] == 1) {
					bufGameStatus->objectives_captured[varInt] = 0;
				} else {
					bufGameStatus->objectives_captured[varInt] = 1;
				}
			}
		}

		bufGameStatus->packet = 8;
		bufGameStatus->game_status = myGameStatus;
		//bufGameStatus->objectives_captured[0] = 1;

		printf("Send packet 8:\n\tgame_status: %d\n\t objectives_captured: ", bufGameStatus->game_status);
		for (i = 0; i < MAX_OBJECTIVES; ++i) {
			printf("%d ", bufGameStatus->objectives_captured[i]);
		}
		printf("\n\n");

		myGameStatus = bufGameStatus->game_status;

		sendto(u, bufGameStatus, sizeof(PKT_GAME_STATUS_U), 0, (struct sockaddr*) &server, sizeof(server));
		break;

	case 9:
		printf("Send:\n\tWe're doing the Bee Gees!! Oooh, oooh, oooh, oooh.....\n");

		send(s, &input, sizeof(packet_t), 0);
		break;

	case 10:
		printf("Enter new x position: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myPosx = varInt;
		}
		printf("Enter new y position: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myPosy = varInt;
		}
		printf("Enter new x velocity: ");
		if (fscanf(stdin, "%lf", &varDub) > 0) {
			myVelx = varDub;
		}
		printf("Enter new y velocity: ");
		if (fscanf(stdin, "%lf", &varDub) > 0) {
			myVely = varDub;
		}

		bufPlayerPos->packet = 10;
		bufPlayerPos->floor = myFloor;
		bufPlayerPos->player_number = myPlayerNumber;
		bufPlayerPos->xPos = myPosx;
		bufPlayerPos->xVel = myVelx;
		bufPlayerPos->yPos = myPosy;
		bufPlayerPos->yVel = myVely;

		printf(
				"Send packet 10:\n\tPlayer number: %d\n\tFloor: %d\n\tnew x position: %d\n\tnew y position: %d\n\tnew x velocity: %f\n\tnew y velocity: %f\n\n",
				bufPlayerPos->player_number, bufPlayerPos->floor, bufPlayerPos->xPos, bufPlayerPos->yPos,
				bufPlayerPos->xVel, bufPlayerPos->yVel);

		sendto(u, bufPlayerPos, sizeof(PKT_POS_UPDATE_U), 0, (struct sockaddr*) &server, sizeof(server));
		break;

	case 12:
		printf("Enter floor to move to: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			newFloor = varInt;
		}
		printf("Enter new x position: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myPosx = varInt;
		}
		printf("Enter new y position: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			myPosy = varInt;
		}

		bufPlayerFloorMove->current_floor = myFloor;
		bufPlayerFloorMove->desired_floor = newFloor;
		bufPlayerFloorMove->player_number = myPlayerNumber;
		bufPlayerFloorMove->desired_xPos = myPosx;
		bufPlayerFloorMove->desired_yPos = myPosy;

		printf(
				"Send packet 12:\n\tPlayer number: %d\n\tCurrent Floor: %d\n\tDesired floor: %d\n\tDesired x position: %d\n\tDesired y position: %d\n\n",
				bufPlayerFloorMove->player_number, bufPlayerFloorMove->current_floor, bufPlayerFloorMove->desired_floor,
				bufPlayerFloorMove->desired_xPos, bufPlayerFloorMove->desired_yPos);

		send(s, &input, sizeof(packet_t), 0);
		send(s, bufPlayerFloorMove, sizeof(PKT_FLOOR_MOVE_REQUEST), 0);
		break;

	case 14:
		printf("Who is doing the tagging: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			tagger = varInt;
		}
		printf("Who is getting tagged: ");
		if (fscanf(stdin, "%d", &varInt) > 0) {
			taggee = varInt;
		}

		bufTag->taggee_id = taggee;
		bufTag->tagger_id = tagger;

		printf("Send packet 14:\n\tTagged player number: %d\n\tTagging player number: %d\n\n", bufTag->taggee_id,
				bufTag->tagger_id);

		send(s, &input, sizeof(packet_t), 0);
		send(s, bufTag, sizeof(PKT_TAGGING), 0);
		break;
	case 99:
		printf("Goodbye!\n\n");
		running = 0;
		break;
	case 0:
	default:
		input = 0;
		break;
	}
	free(bufPlayerJoin);
	free(bufPlayerPos);
	free(bufGameStatus);
	free(bufUpdatePlayer);
	free(bufPlayerFloorMove);
	free(bufTag);
}
void getReply() {
	packet_t response = 0;

//void* packet = malloc(sizeof(packet_t));
//	packet_t type = 0;
//	int received;
	int i;

	PKT_JOIN_RESPONSE *bufJoinResponse = malloc(sizeof(PKT_JOIN_RESPONSE)); //2
	PKT_PLAYERS_UPDATE *bufPlayerUpdate = malloc(sizeof(PKT_PLAYERS_UPDATE)); //3
	PKT_FLOOR_MOVE *bufFloorMove = malloc(sizeof(PKT_FLOOR_MOVE));  //13

	recv(s, &response, sizeof(packet_t), 0);

	switch (response) {
	case 2:
		recv(s, bufJoinResponse, sizeof(PKT_JOIN_RESPONSE), 0);

		printf("Packet 2:\n\tclients_player_number: %d\n\tclients_team_number: %d\n\tconnect_code: %d\n\n",
				bufJoinResponse->clients_player_number, bufJoinResponse->clients_team_number,
				bufJoinResponse->connect_code);
		break;

	case 3:
		recv(s, bufPlayerUpdate, sizeof(PKT_PLAYERS_UPDATE), 0);
		i = myPlayerNumber;

		//for (i = 0; i < 1; ++i) {
		printf("Packet 3 - Player %d:\n\tPlayer_name: %s\n\tPlayer_team: %d\n\tplayer_valid: %d\n\treadystatus: %d\n\n",
				i, bufPlayerUpdate->otherPlayers_name[i], bufPlayerUpdate->otherPlayers_teams[i],
				bufPlayerUpdate->player_valid[i], bufPlayerUpdate->readystatus[i]);
		//	}

		myReadyStatus = bufPlayerUpdate->readystatus[i];
		break;

	case 9:
		printf("We're doing the Bee Gees!! Oooh, oooh, oooh, oooh.....\n");
		break;

	case 13:
		recv(s, bufFloorMove, sizeof(PKT_FLOOR_MOVE), 0);

		printf("Packet 13:\n\tnew_floor: %d\n\txPos: %d\n\tyPos: %d\n\n", bufFloorMove->new_floor, bufFloorMove->xPos,
				bufFloorMove->yPos);

		myFloor = bufFloorMove->new_floor;
		myPosx = bufFloorMove->xPos;
		myPosy = bufFloorMove->yPos;
		break;

	default:
		break;

	}
	free(bufJoinResponse);
	free(bufPlayerUpdate);
	free(bufFloorMove);
//free(packet);

}

void getUDP() {
	packet_t response = 0;

//packet_t type = 0;
	int received = 0;
	int i;

	void* packet = malloc(sizeof(packet_t) + sizeof(PKT_ALL_POS_UPDATE));
	PKT_GAME_STATUS *bufGameStatus = malloc(sizeof(PKT_GAME_STATUS)); //8
	PKT_ALL_POS_UPDATE *bufPlayerAllUpdate = malloc(sizeof(PKT_ALL_POS_UPDATE)); //11

	received = recvfrom(u, packet, sizeof(packet_t) + sizeof(PKT_ALL_POS_UPDATE), 0, (struct sockaddr*) &server,
			(uint*) sizeof(server));
	response = *((packet_t*) packet);

	switch (response) {
	case 8:
		memcpy(bufGameStatus, packet + sizeof(packet_t), sizeof(PKT_GAME_STATUS));

		printf("Packet 8:\n\tgame_status: %d\n\t objectives_captured: ", bufGameStatus->game_status);
		for (i = 0; i < MAX_OBJECTIVES; ++i) {
			printf("%d ", bufGameStatus->objectives_captured[1]);
		}
		printf("\n\n");

		myGameStatus = bufGameStatus->game_status;
		break;

	case 11:
		memcpy(bufPlayerAllUpdate, packet + sizeof(packet_t), sizeof(PKT_ALL_POS_UPDATE));
		i = myPlayerNumber;
		for (i = 0; i < MAX_PLAYERS; ++i) {
			if (bufPlayerAllUpdate->players_on_floor[i] == 1) {
				printf(
						"Packet 11 - player %d:\n\tfloor: %d\n\tplayers_on_floor: %d\n\txPos: %d\n\txVel: %f\n\tyPos: %d\n\tyVel: %f\n\n",
						i, bufPlayerAllUpdate->floor, bufPlayerAllUpdate->players_on_floor[i],
						bufPlayerAllUpdate->xPos[i], bufPlayerAllUpdate->xVel[i], bufPlayerAllUpdate->yPos[i],
						bufPlayerAllUpdate->yVel[i]);
			}
		}

		myFloor = bufPlayerAllUpdate->floor;
		myPosx = bufPlayerAllUpdate->xPos[myPlayerNumber];
		myPosy = bufPlayerAllUpdate->yPos[myPlayerNumber];
		break;

	default:
		break;

	}
	free(bufGameStatus);
	free(bufPlayerAllUpdate);
}

