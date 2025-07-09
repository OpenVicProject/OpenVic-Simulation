#pragma once

#include <random>

#include <fmt/core.h>

#include "openvic-simulation/pathfinding/PointMap.hpp"
#include "openvic-simulation/types/Vector.hpp"
#include "openvic-simulation/utility/Containers.hpp"

#include "Helper.hpp"
#include <snitch/snitch_macros_misc.hpp>

namespace OpenVic::testing {
	template<typename T>
	inline static void _stress_test(const double accuracy_threshold = 0.0000001) {
		// Random stress tests with Floyd-Warshall.
		static constexpr int N = 30;
		std::mt19937 rand { 0 };

		for (int test = 0; test < 1000; test++) {
			INFO(fmt::format("Test := {:>4}", test + 1));
			OpenVic::PointMap pm;
			OpenVic::ivec2_t points[N];
			bool adjacent[N][N] = { { false } };

			// Assign initial coordinates.
			for (int u = 0; u < N; u++) {
				points[u].x = rand() % 100;
				points[u].y = rand() % 100;
				pm.add_point(u, points[u]);
			}
			// Generate a random sequence of operations.
			for (int i = 0; i < 1000; i++) {
				// Pick two different vertices.
				int u, v;
				u = rand() % N;
				v = rand() % (N - 1);
				if (u == v) {
					v = N - 1;
				}
				// Pick a random operation.
				int op = rand();
				switch (op % 9) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					// Add edge (u, v); possibly bidirectional.
					pm.connect_points(u, v, op % 2);
					adjacent[u][v] = true;
					if (op % 2) {
						adjacent[v][u] = true;
					}
					break;
				case 6:
				case 7:
					// Remove edge (u, v); possibly bidirectional.
					pm.disconnect_points(u, v, op % 2);
					adjacent[u][v] = false;
					if (op % 2) {
						adjacent[v][u] = false;
					}
					break;
				case 8:
					// Remove point u and add it back; clears adjacent edges and changes coordinates.
					pm.remove_point(u);
					points[u].x = rand() % 100;
					points[u].y = rand() % 100;
					pm.add_point(u, points[u]);
					for (v = 0; v < N; v++) {
						adjacent[u][v] = adjacent[v][u] = false;
					}
					break;
				}
			}
			// Floyd-Warshall.
			double dist[N][N];
			for (int u = 0; u < N; u++) {
				for (int v = 0; v < N; v++) {
					dist[u][v] = (u == v || adjacent[u][v]) ? std::sqrt(points[u].distance_squared(points[v])) : INFINITY;
				}
			}
			for (int w = 0; w < N; w++) {
				for (int u = 0; u < N; u++) {
					for (int v = 0; v < N; v++) {
						if (dist[u][v] > dist[u][w] + dist[w][v]) {
							dist[u][v] = dist[u][w] + dist[w][v];
						}
					}
				}
			}
			// Display statistics.
			int count = 0;
			for (int u = 0; u < N; u++) {
				for (int v = 0; v < N; v++) {
					if (adjacent[u][v]) {
						count++;
					}
				}
			}
			INFO(fmt::format("{:<3} edges", count));
			count = 0;
			for (int u = 0; u < N; u++) {
				for (int v = 0; v < N; v++) {
					if (!std::isinf(dist[u][v])) {
						count++;
					}
				}
			}
			INFO(fmt::format("{:<3}/{} pairs of reachable points", count - N, N * (N - 1)));

			T a { pm };
			// Check A*'s output.
			bool match = true;
			for (int u = 0; u < N; u++) {
				for (int v = 0; v < N; v++) {
					if (u != v) {
						CAPTURE(u);
						CAPTURE(v);
						memory::vector<PointMap::points_key_type> route = a.get_id_path(u, v);
						if (!std::isinf(dist[u][v])) {
							// Reachable.
							{
								INFO(fmt::format("From {} to {}: Fringe did not find a path", u, v));
								CHECK_FALSE_IF(route.size() == 0) {
									match = false;
									goto exit;
								}
							}
							double astar_dist = 0;
							for (int i = 1; i < route.size(); i++) {
								CAPTURE(i);
								{
									INFO(
										fmt::format("From {} to {}: edge ({}, {}) does not exist", u, v, route[i - 1], route[i])
									);
									CHECK_FALSE_IF(!adjacent[route[i - 1]][route[i]]) {
										match = false;
										goto exit;
									}
								}
								astar_dist += std::sqrt(points[route[i - 1]].distance_squared(points[route[i]]));
							}
							{
								INFO(fmt::format(
									"From {} to {}: Floyd-Warshall gives {:.6}, Fringe gives {:.6}", u, v, dist[u][v],
									astar_dist
								));
								CHECK_FALSE_IF(std::abs(astar_dist - dist[u][v]) >= accuracy_threshold) {
									match = false;
									goto exit;
								}
							}
						} else {
							// Unreachable.
							INFO(fmt::format("From {} to {}: Fringe somehow found a nonexistent path", u, v));
							CHECK_FALSE_IF(route.size() > 0) {
								match = false;
								goto exit;
							}
						}
					}
				}
			}
		exit:
			(void)0;
		}
	}
}
