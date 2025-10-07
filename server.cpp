
#include "Connection.hpp"

#include "hex_dump.hpp"

#include "Game.hpp"

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_map>

#ifdef _WIN32
extern "C" { uint32_t GetACP(); }
#endif
int main(int argc, char **argv) {
#ifdef _WIN32
	{ //when compiled on windows, check that code page is forced to utf-8 (makes file loading/saving work right):
		//see: https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
		uint32_t code_page = GetACP();
		if (code_page == 65001) {
			std::cout << "Code page is properly set to UTF-8." << std::endl;
		} else {
			std::cout << "WARNING: code page is set to " << code_page << " instead of 65001 (UTF-8). Some file handling functions may fail." << std::endl;
		}
	}

	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]);

	//------------ main loop ------------

	//keep track of which connection is controlling which player:
	std::unordered_map< Connection *, Player * > connection_to_player;
	//keep track of game state:
	Game game;
	bool win_broadcasted = false;

	while (true) {
		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(Game::Tick);
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(Game::Tick);
				break;
			}

			//helper used on client close (due to quit) and server close (due to error):
			auto remove_connection = [&](Connection *c) {
				auto f = connection_to_player.find(c);
				assert(f != connection_to_player.end());
				game.remove_player(f->second);
				connection_to_player.erase(f);
			};

			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:

					//create some player info for them:
					connection_to_player.emplace(c, game.spawn_player());

				} else if (evt == Connection::OnClose) {
					//client disconnected:

					remove_connection(c);

				} else { assert(evt == Connection::OnRecv);

					//look up in players list:
					auto f = connection_to_player.find(c);
					assert(f != connection_to_player.end());
					Player &player = *f->second;

					//handle messages from client:
					try {
						bool handled_message;
						do {
							handled_message = false;
							if (player.controls.recv_controls_message(c)) handled_message = true;
							if (c->recv_buffer.size() >= 4 && c->recv_buffer[0] == uint8_t(Message::C2S_Pickup)) {
								uint32_t payload_size = (uint32_t(c->recv_buffer[3]) << 16)
											  | (uint32_t(c->recv_buffer[2]) << 8)
											  |  uint32_t(c->recv_buffer[1]);
								if (c->recv_buffer.size() >= 4 + payload_size) {
										uint8_t type_code = c->recv_buffer[4];
										c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4 + payload_size);
										handled_message = true;
										if (type_code == 0) {
											// carrot
											game.total_carrots_collected += 1;
											std::cout << player.name << " picked a carrot! Total carrots: " << game.total_carrots_collected << std::endl;
											// check win condition
											if (game.total_carrots_collected >= 12 && game.total_tomatoes_collected >= 10 && game.total_beets_collected >= 8) {
												if (!win_broadcasted) {
													for (auto &cp : connection_to_player) {
														Connection *dest = cp.first;
														dest->send(uint8_t(Message::S2C_Win));
														uint32_t size = 0;
														dest->send(uint8_t(size));
														dest->send(uint8_t(size >> 8));
														dest->send(uint8_t(size >> 16));
													}
													win_broadcasted = true;
												}
											}
										} else if (type_code == 1) {
											// carrot seed: gift to the next player (player.id + 1)
											if (!game.players.empty()) {
												std::vector< uint32_t > ids;
												for (auto &p : game.players) ids.push_back(p.id);
												if (!ids.empty()) {
													// find current player's index
													size_t idx = 0;
													bool found = false;
													for (size_t i = 0; i < ids.size(); ++i) {
														if (ids[i] == player.id) { idx = i; found = true; break; }
													}
													size_t target_idx = found ? ((idx + 1) % ids.size()) : 0;
													uint32_t target_id = ids[target_idx];

													Connection *dest = nullptr;
													for (auto &cp : connection_to_player) {
														if (cp.second->id == target_id) { dest = cp.first; break; }
													}
															dest->send(Message::S2C_Gift);
															uint32_t size = 1;
															dest->send(uint8_t(size));
															dest->send(uint8_t(size >> 8));
															dest->send(uint8_t(size >> 16));
															dest->send(uint8_t(0));
															std::cout << player.name << " picked carrot seeds! Sent carrot gift to player " << target_id << std::endl;
												}
											}
										} else if (type_code == 2) {
											// tomato
											game.total_tomatoes_collected += 1;
											std::cout << player.name << " picked a tomato. Total tomatoes: " << game.total_tomatoes_collected << std::endl;
											if (game.total_carrots_collected >= 2 && game.total_tomatoes_collected >= 2 && game.total_beets_collected >= 2) {
												if (!win_broadcasted) {
													for (auto &cp : connection_to_player) {
														Connection *dest = cp.first;
														dest->send(uint8_t(Message::S2C_Win));
														uint32_t size = 0;
														dest->send(uint8_t(size));
														dest->send(uint8_t(size >> 8));
														dest->send(uint8_t(size >> 16));
													}
													win_broadcasted = true;
												}
											}
										} else if (type_code == 3) {
											// tomato seed
											if (!game.players.empty()) {
												std::vector< uint32_t > ids;
												for (auto &p : game.players) ids.push_back(p.id);
												if (!ids.empty()) {
													size_t idx = 0;
													bool found = false;
													for (size_t i = 0; i < ids.size(); ++i) {
														if (ids[i] == player.id) { idx = i; found = true; break; }
													}
													size_t target_idx = found ? ((idx + 1) % ids.size()) : 0;
													uint32_t target_id = ids[target_idx];
													Connection *dest = nullptr;
													for (auto &cp : connection_to_player) {
														if (cp.second->id == target_id) { dest = cp.first; break; }
													}
													dest->send(Message::S2C_Gift);
													uint32_t size = 1;
													dest->send(uint8_t(size));
													dest->send(uint8_t(size >> 8));
													dest->send(uint8_t(size >> 16));
													dest->send(uint8_t(2));
													std::cout <<  player.name << " picked tomato seeds! Sent tomato gift to player id " << target_id << std::endl;
												}
											}
										} else if (type_code == 4) {
											// beet
											game.total_beets_collected += 1;
											std::cout << player.name << " picked a beet! Total beets: " << game.total_beets_collected << std::endl;
											if (game.total_carrots_collected >= 2 && game.total_tomatoes_collected >= 2 && game.total_beets_collected >= 2) {
												if (!win_broadcasted) {
													for (auto &cp : connection_to_player) {
														Connection *dest = cp.first;
														dest->send(uint8_t(Message::S2C_Win));
														uint32_t size = 0;
														dest->send(uint8_t(size));
														dest->send(uint8_t(size >> 8));
														dest->send(uint8_t(size >> 16));
													}
													win_broadcasted = true;
												}
											}
										} else if (type_code == 5) {
											// beet seed
											if (!game.players.empty()) {
												std::vector< uint32_t > ids;
												for (auto &p : game.players) ids.push_back(p.id);
												if (!ids.empty()) {
													size_t idx = 0;
													bool found = false;
													for (size_t i = 0; i < ids.size(); ++i) {
														if (ids[i] == player.id) { idx = i; found = true; break; }
													}
													size_t target_idx = found ? ((idx + 1) % ids.size()) : 0;
													uint32_t target_id = ids[target_idx];
													Connection *dest = nullptr;
													for (auto &cp : connection_to_player) {
														if (cp.second->id == target_id) { dest = cp.first; break; }
													}
														dest->send(Message::S2C_Gift);
														uint32_t size = 1;
														dest->send(uint8_t(size));
														dest->send(uint8_t(size >> 8));
														dest->send(uint8_t(size >> 16));
														dest->send(uint8_t(4));
														std::cout << player.name << " picked beet seeds! Sent beet gift to player id " << target_id << std::endl;
												}
											}
										}
									}
							}
							//TODO: extend for more message types as needed
						} while (handled_message);
					} catch (std::exception const &e) {
						std::cout << "Disconnecting client:" << e.what() << std::endl;
						c->close();
						remove_connection(c);
					}
				}
			}, remain);
		}

		//update current game state
		game.update(Game::Tick);

		//send updated game state to all clients
		for (auto &[c, player] : connection_to_player) {
			game.send_state_message(c, player);
		}

	}


	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
