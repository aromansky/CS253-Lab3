#include <iostream>
#include <vector>
#include <intrin.h>

using namespace std;
using Board = uint64_t;

// константы границ доски
const Board H_FILE = 0x8080808080808080ULL;
const Board A_FILE = 0x0101010101010101ULL;

int char_to_col_index(char col) {
	if (col >= 'a' && col <= 'h') {
		return col - 'a';
	}
	if (col >= 'A' && col <= 'H') {
		return col - 'A';
	}
	return -1;
}

struct GameState {
    Board white_pieces;
    Board black_pieces;

	GameState() : white_pieces(0), black_pieces(0) {
		for(int row = 1; row <= 3; row++) {
			for(char col = 'a'; col <= 'd'; col++) {
				set_piece(true, row, col);
			}

			for (char col = 'e'; col <= 'h'; col++) {
				set_piece(false, 9 - row, col);
			}
		}
	}

	GameState(Board white, Board black) : white_pieces(white), black_pieces(black) {}

	vector<GameState> get_successors(bool is_white_turn) const {
		vector<GameState> successors;

		const uint64_t friendly_pieces = is_white_turn ? white_pieces : black_pieces;
		const uint64_t all_pieces = white_pieces | black_pieces;
		const uint64_t empty_squares = ~all_pieces;

		uint64_t pieces_to_move = friendly_pieces;
		while (pieces_to_move != 0) {
			unsigned long start_pos;
			_BitScanForward64(&start_pos, pieces_to_move);
			uint64_t start_mask = 1ULL << start_pos;

			generate_single_moves(successors, start_mask, start_pos, is_white_turn, all_pieces, empty_squares);
			generate_jumps(successors, start_mask, start_pos, is_white_turn, all_pieces, empty_squares);

			pieces_to_move &= ~start_mask;
		}

		return successors;
	}

	bool is_goal_state() const {
		return black_pieces == 17361640446303928320 && white_pieces == 986895;
	}

	// штуки для хэшмапов и сравненийй 

	bool operator==(const GameState& other) const {
		return white_pieces == other.white_pieces &&
			black_pieces == other.black_pieces;
	}
private:
	void set_piece(bool is_white, int row, char col) {
		if (is_white) {
			white_pieces |= (1ULL << ((row - 1) * 8 + char_to_col_index(col)));
		}
		else {
			black_pieces |= (1ULL << ((row - 1) * 8 + char_to_col_index(col)));
		}
	}

	/// <summary>
	/// Выполнение хода
	/// </summary>
	GameState make_move(int start_pos, int end_pos, bool is_white_turn) const {
		Board new_white = white_pieces;
		Board new_black = black_pieces;

		uint64_t start_mask = 1ULL << start_pos;
		uint64_t end_mask = 1ULL << end_pos;

		if (is_white_turn) {
			new_white ^= start_mask;
			new_white |= end_mask;
		}
		else {
			new_black ^= start_mask;
			new_black |= end_mask;
		}
		return GameState(new_white, new_black);
	}

	/// <summary>
	/// Генерация ходов на 1 клетку
	/// </summary>
	void generate_single_moves(std::vector<GameState>& successors, uint64_t start_mask, int start_pos, bool is_white_turn, uint64_t all_pieces, uint64_t empty_squares) const {
		uint64_t potential_targets;

		// Влево
		potential_targets = (start_mask >> 1) & (~H_FILE);
		if (potential_targets & empty_squares) {
			successors.push_back(make_move(start_pos, start_pos - 1, is_white_turn));
		}

		// Вправо
		potential_targets = (start_mask << 1) & (~A_FILE);
		if (potential_targets & empty_squares) {
			successors.push_back(make_move(start_pos, start_pos + 1, is_white_turn));
		}

		// Вниз
		potential_targets = start_mask >> 8;
		if (potential_targets & empty_squares) {
			successors.push_back(make_move(start_pos, start_pos - 8, is_white_turn));
		}

		// Вверх
		potential_targets = start_mask << 8;
		if (potential_targets & empty_squares) {
			successors.push_back(make_move(start_pos, start_pos + 8, is_white_turn));
		}
	}

	/// <summary>
	/// прыжок через одну шашку
	/// </summary>
	void generate_jumps(std::vector<GameState>& successors, uint64_t start_mask, int start_pos, bool is_white_turn, uint64_t all_pieces, uint64_t empty_squares) const {
		uint64_t target_mask;
		uint64_t mid_mask;


		// влево
		mid_mask = start_mask >> 1;
		target_mask = start_mask >> 2;
		if ((mid_mask & all_pieces) && (target_mask & empty_squares) && !(mid_mask & H_FILE) && !(target_mask & H_FILE) && !(target_mask & (H_FILE >> 1))) {
			successors.push_back(make_move(start_pos, start_pos - 2, is_white_turn));
		}

		// вправо
		mid_mask = start_mask << 1;
		target_mask = start_mask << 2;
		if ((mid_mask & all_pieces) && (target_mask & empty_squares) && !(mid_mask & A_FILE) && !(target_mask & A_FILE) && !(target_mask & (A_FILE << 1))) {
			successors.push_back(make_move(start_pos, start_pos + 2, is_white_turn));
		}


		// вниз
		mid_mask = start_mask >> 8;
		target_mask = start_mask >> 16;
		if ((mid_mask & all_pieces) && (target_mask & empty_squares) && (start_pos >= 16)) {
			successors.push_back(make_move(start_pos, start_pos - 16, is_white_turn));
		}

		// вверх
		mid_mask = start_mask << 8;
		target_mask = start_mask << 16;
		if ((mid_mask & all_pieces) && (target_mask & empty_squares) && (start_pos <= 47)) {
			successors.push_back(make_move(start_pos, start_pos + 16, is_white_turn));
		}
	}
};

struct GameStateHash {
	size_t operator()(GameState const& s) const noexcept {
		return std::hash<uint64_t>()(
			s.white_pieces * 1315423911ULL ^ (s.black_pieces + 0x9e3779b97f4a7c15ULL));
	}
};




int main() {
	GameState state;

	cout << state.black_pieces << endl;
	cout << state.white_pieces << endl;
	return 0;
}