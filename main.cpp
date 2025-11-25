#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <stack>
#include <cmath>
#include <chrono>
#include <functional>
#include <intrin.h>

using namespace std;
using Board = uint64_t;

// константы границ доски
const Board H_FILE = 0x2020202020202020ULL;
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
			for(char col = 'a'; col <= 'c'; col++) {
				set_piece(true, row, col);
			}

			for (char col = 'd'; col <= 'f'; col++) {
				set_piece(false, 7 - row, col);
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
		return black_pieces == 29127 && white_pieces == 61083746304;
	}

	// штуки для хэшмапов и сравненийй 

	bool operator==(const GameState& other) const {
		return white_pieces == other.white_pieces &&
			black_pieces == other.black_pieces;
	}
private:
	void set_piece(bool is_white, int row, char col) {
		if (is_white) {
			white_pieces |= (1ULL << ((row - 1) * 6 + char_to_col_index(col)));
		}
		else {
			black_pieces |= (1ULL << ((row - 1) * 6 + char_to_col_index(col)));
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
		potential_targets = start_mask >> 6;
		if (potential_targets & empty_squares) {
			successors.push_back(make_move(start_pos, start_pos - 6, is_white_turn));
		}

		// Вверх
		potential_targets = start_mask << 6;
		if (potential_targets & empty_squares) {
			successors.push_back(make_move(start_pos, start_pos + 6, is_white_turn));
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
		mid_mask = start_mask >> 6;
		target_mask = start_mask >> 12;
		if ((mid_mask & all_pieces) && (target_mask & empty_squares) && (start_pos >= 16)) {
			successors.push_back(make_move(start_pos, start_pos - 12, is_white_turn));
		}

		// вверх
		mid_mask = start_mask << 6;
		target_mask = start_mask << 12;
		if ((mid_mask & all_pieces) && (target_mask & empty_squares) && (start_pos <= 47)) {
			successors.push_back(make_move(start_pos, start_pos + 12, is_white_turn));
		}
	}
};

struct GameStateHash {
	size_t operator()(GameState const& s) const noexcept {
		return std::hash<uint64_t>()(
			s.white_pieces * 1315423911ULL ^ (s.black_pieces + 0x9e3779b97f4a7c15ULL));
	}
};

void print_state(const GameState& state) {
	cout << "  A B C D E F\n";
	for (int r = 6; r >= 1; --r) {
		cout << r << " ";
		for (char c = 'A'; c <= 'F'; ++c) {
			int pos = (r - 1) * 6 + char_to_col_index(c);
			uint64_t mask = 1ULL << pos;

			if (state.white_pieces & mask) {
				cout << "W ";
			}
			else if (state.black_pieces & mask) {
				cout << "B ";
			}
			else {
				cout << ". ";
			}
		}
		cout << "\n";
	}
	cout << "\n";
}

void print_path(const GameState& start, const GameState& goal, const unordered_map<GameState, GameState, GameStateHash>& parent_map) {
	vector<GameState> path;
	GameState current = goal;

	// Проверка на случай, если целевое состояние - это стартовое
	if (current == start && parent_map.find(current) == parent_map.end()) {
		path.push_back(start);
	}
	else {
		while (!(current == start)) {
			path.push_back(current);
			if (parent_map.find(current) == parent_map.end()) {
				// Ошибка: путь не найден до конца
				cout << "Ошибка: путь не прослежен до стартового состояния.\n";
				return;
			}
			current = parent_map.at(current);
		}
		path.push_back(start);
	}

	reverse(path.begin(), path.end());

	cout << "\nНайден кратчайший путь за " << path.size() - 1 << " ходов.\n";
	for (size_t i = 0; i < path.size(); ++i) {
		cout << "--- Ход " << i << " ---\n";
		print_state(path[i]);
	}
}

int heuristic(const GameState& state, bool is_white_turn) {
	int total_distance = 0;

	// Целевая область для белых: D4-F6 (верхний правый угол 3x3) <--- ИСПРАВЛЕНО
	const int white_target_rows[] = { 4, 5, 6 }; // Ряды 4, 5, 6
	const int white_target_cols[] = { 3, 4, 5 }; // Колонки D, E, F

	// Целевая область для черных: A1-C3 (нижний левый угол 3x3) <--- ИСПРАВЛЕНО
	const int black_target_rows[] = { 1, 2, 3 }; // Ряды 1, 2, 3
	const int black_target_cols[] = { 0, 1, 2 }; // Колонки A, B, C

	// Итерация по белым шашкам
	uint64_t current_white = state.white_pieces;
	while (current_white != 0) {
		unsigned long current_pos;
		_BitScanForward64(&current_pos, current_white);
		current_white &= ~(1ULL << current_pos);

		int current_row = current_pos / 6 + 1;
		int current_col = current_pos % 6;

		int min_dist = 100;

		// Расчет минимального расстояния до любого целевого места
		for (int tr : white_target_rows) {
			for (int tc : white_target_cols) {
				int dist = abs(tr - current_row) + abs(tc - current_col);
				min_dist = min(min_dist, dist);
			}
		}
		total_distance += min_dist;
	}

	// Итерация по черным шашкам
	uint64_t current_black = state.black_pieces;
	while (current_black != 0) {
		unsigned long current_pos;
		_BitScanForward64(&current_pos, current_black);
		current_black &= ~(1ULL << current_pos);

		int current_row = current_pos / 6 + 1;
		int current_col = current_pos % 6;

		int min_dist = 100;

		for (int tr : black_target_rows) {
			for (int tc : black_target_cols) {
				int dist = abs(tr - current_row) + abs(tc - current_col);
				min_dist = min(min_dist, dist);
			}
		}
		total_distance += min_dist;
	}

	return total_distance;
}


void DFS(const GameState& start_state, int max_depth) {
	// В "Уголках" нет проверки на решаемость, как в "15"

	unordered_map<GameState, GameState, GameStateHash> parent_map;
	stack<pair<GameState, int>> s; // stack<StateDepth>

	s.push({ start_state, 0 });
	parent_map[start_state] = start_state; // Указываем, что стартовое состояние не имеет родителя (или родитель - оно само)

	// Определяем, кто ходит первым
	bool is_white_turn = true;

	while (!s.empty()) {
		GameState current_state = s.top().first;
		int current_depth = s.top().second;
		s.pop();

		if (current_state.is_goal_state()) {
			cout << "Решение найдено с помощью DFS!" << endl;
			print_path(start_state, current_state, parent_map);
			return;
		}

		if (current_depth >= max_depth) {
			continue;
		}

		// Чётная глубина (0, 2, 4...) -> Ходит White (если White начал)
		// Нечётная глубина (1, 3, 5...) -> Ходит Black
		is_white_turn = (current_depth % 2 == 0);

		for (const GameState& next_state : current_state.get_successors(is_white_turn)) {
			if (parent_map.find(next_state) == parent_map.end()) {
				parent_map[next_state] = current_state;
				s.push({ next_state, current_depth + 1 });
			}
		}
	}

	cout << "Решение не найдено в пределах глубины " << max_depth << "." << endl;
}

bool DLS(const GameState& start_state, int max_depth, GameState& solution_end_node, unordered_map<GameState, GameState, GameStateHash>& parent_map) {
	parent_map.clear();
	stack<pair<GameState, int>> s;
	s.push({ start_state, 0 });
	parent_map[start_state] = start_state;

	while (!s.empty()) {
		GameState current_state = s.top().first;
		int current_depth = s.top().second;
		s.pop();

		if (current_state.is_goal_state()) {
			solution_end_node = current_state;
			return true;
		}

		if (current_depth >= max_depth) {
			continue;
		}

		bool is_white_turn = (current_depth % 2 == 0);

		for (const GameState& next_state : current_state.get_successors(is_white_turn)) {
			if (parent_map.find(next_state) == parent_map.end()) {
				parent_map[next_state] = current_state;
				s.push({ next_state, current_depth + 1 });
			}
		}
	}
	return false;
}

bool IDS(const GameState& start_state, int absolute_max_depth, GameState& solution_end_node, unordered_map<GameState, GameState, GameStateHash>& parent_map) {
	cout << "--- Запуск IDS до глубины: " << absolute_max_depth << " ---\n";

	for (int depth = 0; depth <= absolute_max_depth; ++depth) {
		cout << "Текущая глубина: " << depth << endl;

		if (DLS(start_state, depth, solution_end_node, parent_map)) {
			cout << "Решение найдено на глубине " << depth << " с помощью IDS.\n";
			return true;
		}
	}

	return false;
}


struct Node {
	GameState state;
	int g;
	int h;
	int f;

	Node(const GameState& s, int g_cost, int h_cost)
		: state(s), g(g_cost), h(h_cost), f(g_cost + h_cost) {
	}

	bool operator>(const Node& other) const {
		if (f == other.f) {
			return h > other.h;
		}
		return f > other.f;
	}
};

bool A_star(const GameState& start_state, GameState& solution_end_node, unordered_map<GameState, GameState, GameStateHash>& parent_map) {
	parent_map.clear();

	priority_queue<Node, vector<Node>, greater<Node>> open_list;
	// g_scores хранит ЛУЧШУЮ известную стоимость g для каждого состояния
	unordered_map<GameState, int, GameStateHash> g_scores;

	// Определяем, кто ходит первым
	bool is_white_turn = true;

	int h_start = heuristic(start_state, is_white_turn);
	Node start_node(start_state, 0, h_start);

	open_list.push(start_node);
	g_scores[start_state] = 0;
	parent_map[start_state] = start_state;

	while (!open_list.empty()) {
		Node current_node = open_list.top();
		open_list.pop();
		GameState current_state = current_node.state;

		// Эта проверка необходима, т.к. в open_list могут быть старые (более дорогие) пути
		if (current_node.g > g_scores.at(current_state)) {
			continue;
		}

		if (current_state.is_goal_state()) {
			solution_end_node = current_state;
			cout << "Решение найдено с помощью A*.\n";
			return true;
		}

		is_white_turn = (current_node.g % 2 != 0); // Следующий ход

		for (const GameState& next_state : current_state.get_successors(is_white_turn)) {
			int new_g = current_node.g + 1;

			// Если состояние не посещалось ИЛИ мы нашли более короткий путь
			if (g_scores.find(next_state) == g_scores.end() || new_g < g_scores.at(next_state)) {

				bool next_is_white_turn = !is_white_turn; // Следующий игрок
				int h_next = heuristic(next_state, next_is_white_turn);
				Node next_node(next_state, new_g, h_next);

				g_scores[next_state] = new_g;
				parent_map[next_state] = current_state;

				open_list.push(next_node);
			}
		}
	}

	return false;
}

int main() {
	setlocale(LC_ALL, "Russian");

	GameState start_state; // Начальное состояние "Уголков"
	GameState solution_end_node;
	unordered_map<GameState, GameState, GameStateHash> solution_parent_map;

	const int MAX_DEPTH_SEARCH = 40; // Разумное ограничение для IDS/DFS
	const int ITERATIONS = 1;

	// --- Пример использования ---

	cout << "Начальное состояние:\n";
	print_state(start_state);

	cout << endl;

	for (GameState g: start_state.get_successors(true))
		print_state(g);

	//// 1. DFS
	//cout << "\n==============================\n";
	//cout << "Запуск DFS...\n";
	//DFS(start_state, MAX_DEPTH_SEARCH);

	//// 2. IDS
	/*cout << "\n==============================\n";
	cout << "Запуск IDS...\n";
	if (IDS(start_state, MAX_DEPTH_SEARCH, solution_end_node, solution_parent_map)) {
		print_path(start_state, solution_end_node, solution_parent_map);
	}
	else {
		cout << "IDS не нашел решения в пределах максимальной глубины.\n";
	}*/

	// 3. A*
	/*cout << "\n==============================\n";
	cout << "Запуск A*...\n";
	solution_parent_map.clear();
	if (A_star(start_state, solution_end_node, solution_parent_map)) {
		print_path(start_state, solution_end_node, solution_parent_map);
	}
	else {
		cout << "A* не нашел решения.\n";
	}*/

	return 0;
}