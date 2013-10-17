#include "player_map/rosmap.hpp"

#include <cmath>

#include <ros/ros.h>

#include <nav_msgs/GetMap.h>

using namespace std;
namespace scarab {

double pathLength(const Path &path) {
  double dist = 0.0;
  for (size_t i = 0; i < path.size() - 1; ++i) {
    dist += (path[i] - path[i+1]).norm();
  }
  return dist;
}

void convertMap(const nav_msgs::OccupancyGrid &map, map_t *pmap,
    const int free_threshold, const int occupied_threshold) {
  pmap->size_x = map.info.width;
  pmap->size_y = map.info.height;
  pmap->scale = map.info.resolution;
  pmap->origin_x = map.info.origin.position.x + (pmap->size_x / 2) * pmap->scale;
  pmap->origin_y = map.info.origin.position.y + (pmap->size_y / 2) * pmap->scale;
  pmap->max_occ_dist = 0.0;
  // Convert to player format
  pmap->cells = (map_cell_t*)malloc(sizeof(map_cell_t)*pmap->size_x*pmap->size_y);
  ROS_ASSERT(pmap->cells);
  for(int i=0;i<pmap->size_x * pmap->size_y;i++) {
    if(map.data[i] >= 0 && map.data[i] <= free_threshold) {
      pmap->cells[i].occ_state = -1;
    } else if(map.data[i] >= occupied_threshold) {
      pmap->cells[i].occ_state = +1;
    } else {
      pmap->cells[i].occ_state = 0;
    }

    pmap->cells[i].occ_dist = 0;
  }
}


map_t * requestCSpaceMap(const char *srv_name, const int free_threshold,
                         const int occupied_threshold) {
  ros::NodeHandle nh;
  // Taken from PAMCL
  map_t* map = map_alloc();
  ROS_ASSERT(map);

  // get map via RPC
  nav_msgs::GetMap::Request  req;
  nav_msgs::GetMap::Response resp;
  ROS_INFO("Requesting the map...");
  while(!ros::service::call(srv_name, req, resp)) {
    ROS_WARN("Request for map '%s' failed; trying again...",
             ros::names::resolve(string(srv_name)).c_str());
    ros::Duration d(4.0);
    d.sleep();
    if (!nh.ok()) {
      return NULL;
    }
  }
  ROS_INFO_ONCE("Received a %d X %d map @ %.3f m/pix\n",
                resp.map.info.width, resp.map.info.height,
                resp.map.info.resolution);

  convertMap(resp.map, map, free_threshold, occupied_threshold);
  return map;
}


OccupancyMap::OccupancyMap() : map_(NULL), ncells_(0),
  min_occupied_threshold_(100), max_free_threshold_(0) {

}

OccupancyMap::~OccupancyMap() {
  if (map_ != NULL) {
    map_free(map_);
  }
}

OccupancyMap* OccupancyMap::FromMapServer(const char *srv_name,
    const int free_threshold, const int occupied_threshold) {
  map_t *map = requestCSpaceMap(srv_name, free_threshold, occupied_threshold);
  OccupancyMap *occ_map = new OccupancyMap();
  occ_map->setMap(map);
  return occ_map;
}

void OccupancyMap::setMap(map_t *map) {
  if (map_ != NULL) {
    map_free(map_);
  }
  map_ = map;
}

void OccupancyMap::setMap(const nav_msgs::OccupancyGrid &grid) {
  if (map_ != NULL) {
    map_free(map_);
  }
  map_ = map_alloc();
  ROS_ASSERT(map_);
  convertMap(grid, map_, max_free_threshold_, min_occupied_threshold_);
}

void OccupancyMap::updateCSpace(double max_occ_dist) {
  // check if cspace needs to be updated
  if (map_ == NULL || map_->max_occ_dist > max_occ_dist) {
    return;
  }
  map_update_cspace(map_, max_occ_dist);
}

inline double OccupancyMap::minX() {
  return MAP_WXGX(map_, 0);
}

inline double OccupancyMap::minY() {
  return MAP_WYGY(map_, 0);
}

inline double OccupancyMap::maxX() {
  return MAP_WXGX(map_, map_->size_x);
}

inline double OccupancyMap::maxY() {
  return MAP_WYGY(map_, map_->size_y);
}

const map_cell_t* OccupancyMap::getCell(double x, double y) {
  return map_get_cell(map_, x, y, 0);
}

bool OccupancyMap::lineOfSight(double x1, double y1, double x2, double y2,
                               double max_occ_dist /* = 0.0 */,
                               bool allow_unknown /* = false */) const {
  if (map_ == NULL) {
    return true;
  }
  if (map_->max_occ_dist < max_occ_dist) {
    ROS_ERROR("OccupancyMap::lineOfSight() CSpace has been calculated up to %f, "
              "but max_occ_dist=%.2f",
              map_->max_occ_dist, max_occ_dist);
    ROS_BREAK();
  }
  // March along the line between (x1, y1) and (x2, y2) until the point passes
  // beyond (x2, y2).
  double step_size = map_->scale / 2.0;

  double dy = y2 - y1;
  double dx = x2 - x1;
  double t = atan2(dy, dx);
  double ct = cos(t);
  double st = sin(t);

  double mag_sq = dy * dy + dx * dx;
  // fprintf(stderr, "t: %f ct: %f st: %f\n", t, ct, st);
  double line_x = x1, line_y = y1;
  // Terminate when current point defines a vector longer than original vector
  while (pow(line_x - x1, 2) + pow(line_y - y1, 2) < mag_sq) {
    // fprintf(stderr, "%f %f\n", line_x, line_y);
    map_cell_t *cell = map_get_cell(map_, line_x, line_y, 0);
    if (cell == NULL) {
      // ROS_WARN_THROTTLE(5, "lineOfSight() Beyond map edge");
      return false;
    } else if (cell->occ_state == +1 || (!allow_unknown && cell->occ_state == 0) || cell->occ_dist < max_occ_dist) {
      return false;
    }
    line_x += step_size * ct;
    line_y += step_size * st;
  }
  return true;
}

void OccupancyMap::initializeSearch(double startx, double starty) {
  starti_ = MAP_GXWX(map_, startx);
  startj_ = MAP_GYWY(map_, starty);

  if (!MAP_VALID(map_, starti_, startj_)) {
    ROS_ERROR("OccupancyMap::initializeSearch() Invalid starting position");
    ROS_BREAK();
  }

  int ncells = map_->size_x * map_->size_y;
  if (ncells_ != ncells) {
    ncells_ = ncells;
    costs_.reset(new float[ncells]);
    prev_i_.reset(new int[ncells]);
    prev_j_.reset(new int[ncells]);
  }

  // TODO: Return to more efficient lazy-initialization
  // // Map is large and initializing costs_ takes a while.  To speedup,
  // // partially initialize costs_ in a rectangle surrounding start and stop
  // // positions + margin.  If you run up against boundary, initialize the rest.
  // int margin = 120;
  // init_ul_ = make_pair(max(0, min(starti_, stopi) - margin),
  //                      max(0, min(startj_, stopj) - margin));
  // init_lr_ = make_pair(min(map_->size_x, max(starti_, stopi) + margin),
  //                      min(map_->size_y, max(startj_, stopj) + margin));
  // for (int j = init_ul.second; j < init_lr.second; ++j) {
  //   for (int i = init_ul.first; i < init_lr.first; ++i) {
  //     int ind = MAP_INDEX(map_, i, j);
  //     costs_[ind] = std::numeric_limits<float>::infinity();
  //   }
  // }
  // full_init_ = false;

  for (int i = 0; i < ncells_; ++i) {
    costs_[i] = std::numeric_limits<float>::infinity();
    prev_i_[i] = -1;
    prev_j_[i] = -1;
  }

  int start_ind = MAP_INDEX(map_, starti_, startj_);
  costs_[start_ind] = 0.0;
  prev_i_[starti_] = starti_;
  prev_j_[startj_] = startj_;

  Q_.reset(new set<Node, NodeCompare>());
  Q_->insert(Node(make_pair(starti_, startj_), 0.0, 0.0));

  stopi_ = -1;
  stopj_ = -1;
}

void OccupancyMap::addNeighbors(const Node &node, double max_occ_dist, bool allow_unknown) {
  // TODO: Return to more efficient lazy-initialization
  // // Check if we're neighboring nodes whose costs_ are uninitialized.
  // // if (!full_init &&
  // //     ((ci + 1 >= init_lr.first) || (ci - 1 <= init_ul.first) ||
  // //      (cj + 1 >= init_lr.second) || (cj - 1 <= init_ul.second))) {
  // //   full_init = true;
  // //   for (int j = 0; j < map_->size_y; ++j) {
  // //     for (int i = 0; i < map_->size_x; ++i) {
  // //       // Only initialize costs_ that are outside original rectangle
  // //       if (!(init_ul.first <= i && i < init_lr.first &&
  // //             init_ul.second <= j && j < init_lr.second)) {
  // //         int ind = MAP_INDEX(map_, i, j);
  // //         costs_[ind] = std::numeric_limits<float>::infinity();
  // //       }
  // //     }
  // //   }
  // // }

  int ci = node.coord.first;
  int cj = node.coord.second;

  // Iterate over neighbors
  for (int newj = cj - 1; newj <= cj + 1; ++newj) {
    for (int newi = ci - 1; newi <= ci + 1; ++newi) {
      // Skip self edges
      if ((newi == ci && newj == cj) || !MAP_VALID(map_, newi, newj)) {
        continue;
      }
      // fprintf(stderr, "  Examining %i %i ", newi, newj);
      int index = MAP_INDEX(map_, newi, newj);
      map_cell_t *cell = map_->cells + index;
      // If cell is occupied or too close to occupied cell, continue

      if (cell->occ_state == +1 || (!allow_unknown && cell->occ_state == 0) || cell->occ_dist < max_occ_dist) {
        // fprintf(stderr, "occupado\n");
        continue;
      }
      // fprintf(stderr, "free\n");
      double edge_cost = ci == newi || cj == newj ? 1 : sqrt(2);
      double heur_cost = 0.0;
      if (stopi_ != -1 && stopj_ != -1) {
        heur_cost = hypot(newi - stopi_, newj - stopj_);
      }
      double ttl_cost = edge_cost + node.true_dist + heur_cost;
      if (ttl_cost < costs_[index]) {
        // fprintf(stderr, "    Better path: new cost= % 6.2f\n", ttl_cost);
        // If node has finite cost, it's in queue and needs to be removed
        if (!isinf(costs_[index])) {
          Q_->erase(Node(make_pair(newi, newj), costs_[index], 0.0));
        }
        costs_[index] = ttl_cost;
        prev_i_[index] = ci;
        prev_j_[index] = cj;
        Q_->insert(Node(make_pair(newi, newj),
                        edge_cost + node.true_dist,
                        ttl_cost));
      }
    }
  }
}

void OccupancyMap::buildPath(int i, int j, Path *path) {
  while (!(i == starti_ && j == startj_)) {
    int index = MAP_INDEX(map_, i, j);
    float x = MAP_WXGX(map_, i);
    float y = MAP_WYGY(map_, j);
    path->push_back(Eigen::Vector2f(x, y));

    i = prev_i_[index];
    j = prev_j_[index];
  }
  float x = MAP_WXGX(map_, i);
  float y = MAP_WYGY(map_, j);
  path->push_back(Eigen::Vector2f(x, y));
}

bool OccupancyMap::nextNode(double max_occ_dist, Node *curr_node, bool allow_unknown) {
  if (!Q_->empty()) {
    // Copy node and then erase it
    *curr_node = *Q_->begin();
    int ci = curr_node->coord.first, cj = curr_node->coord.second;
    // fprintf(stderr, "At %i %i (cost = %6.2f)  % 7.2f % 7.2f \n",
    //     ci, cj, curr_node.true_dist, MAP_WXGX(map_, ci), MAP_WYGY(map_, cj));
    costs_[MAP_INDEX(map_, ci, cj)] = curr_node->true_dist;
    Q_->erase(Q_->begin());
    addNeighbors(*curr_node, max_occ_dist, allow_unknown);
    return true;
  } else {
    return false;
  }
}

Path OccupancyMap::astar(double startx, double starty,
                                double stopx, double stopy,
                                double max_occ_dist /* = 0.0 */,
                                bool allow_unknown /* = false */) {
  Path path;

  if (map_ == NULL) {
    ROS_WARN("OccupancyMap::astar() Map not set");
    return path;
  }

  int stopi = MAP_GXWX(map_, stopx), stopj = MAP_GYWY(map_, stopy);
  if (!MAP_VALID(map_, stopi ,stopj)) {
    ROS_ERROR("OccupancyMap::astar() Invalid stopping position");
    ROS_BREAK();
  }
  if (map_->max_occ_dist < max_occ_dist) {
    ROS_ERROR("OccupancyMap::astar() CSpace has been calculated up to %f, "
              "but max_occ_dist=%.2f",
              map_->max_occ_dist, max_occ_dist);
    ROS_BREAK();
  }

  initializeSearch(startx, starty);
  // Set stop to use heuristic
  stopi_ = stopi;
  stopj_ = stopj;

  bool found = false;
  Node curr_node;
  while (nextNode(max_occ_dist, &curr_node, allow_unknown)) {
    if (curr_node.coord.first == stopi && curr_node.coord.second == stopj) {
      found = true;
      break;
    }
  }

  // Recreate path
  if (found) {
    buildPath(stopi, stopj, &path);
  }
  return Path(path.rbegin(), path.rend());
}

const Path&
OccupancyMap::prepareShortestPaths(double x, double y, double distance,
                                   double margin, double max_occ_dist,
                                   double min_dist, bool allow_unknown) {
  endpoints_.clear();

  if (map_ == NULL) {
    ROS_WARN("OccupancyMap::prepareShortestPaths() Map not set");
    return endpoints_;
  }

  if (map_->max_occ_dist < max_occ_dist) {
    ROS_ERROR("OccupancyMap::prepareShortestPaths() CSpace has been calculated "
              "up to %f, but max_occ_dist=%.2f",
              map_->max_occ_dist, max_occ_dist);
    ROS_BREAK();
  }

  initializeSearch(x, y);

  Node curr_node;
  while (nextNode(max_occ_dist, &curr_node, allow_unknown)) {
    double node_dist = curr_node.true_dist * map_->scale;
    if (fabs(node_dist - distance) < margin && node_dist > min_dist) {
      float x = MAP_WXGX(map_, curr_node.coord.first);
      float y = MAP_WYGY(map_, curr_node.coord.second);
      endpoints_.push_back(Eigen::Vector2f(x, y));
    } else if (node_dist > distance + margin) {
      break;
    }
  }
  return endpoints_;
}

Path OccupancyMap::buildShortestPath(int ind) {
  Path path;

  if (map_ == NULL) {
    ROS_WARN("OccupancyMap::buildShortestPath() Map not set");
    return path;
  }

  // Recreate path
  const Eigen::Vector2f &stop = endpoints_.at(ind);
  int stopi = MAP_GXWX(map_, stop(0)), stopj = MAP_GYWY(map_, stop(1));
  buildPath(stopi, stopj, &path);
  return Path(path.rbegin(), path.rend());
}

void OccupancyMap::prepareAllShortestPaths(double x, double y,
                                           double max_occ_dist,
                                           bool allow_unknown) {
  if (map_ == NULL) {
    ROS_WARN("OccupancyMap::prepareAllShortestPaths() Map not set");
    return;
  }

  if (map_->max_occ_dist < max_occ_dist) {
    ROS_ERROR("OccupancyMap::shortestToDests() CSpace has been calculated "
              "up to %f, but max_occ_dist=%.2f",
              map_->max_occ_dist, max_occ_dist);
    ROS_BREAK();
  }

  initializeSearch(x, y);

  Node curr_node;
  while (nextNode(max_occ_dist, &curr_node, allow_unknown)) {
    ;
  }
}

Path OccupancyMap::shortestPath(double stopx, double stopy) {
  Path path;

  if (map_ == NULL) {
    ROS_WARN("OccupancyMap::shortestPath() Map not set");
    return path;
  }

  int i = MAP_GXWX(map_, stopx);
  int j = MAP_GYWY(map_, stopy);
  int ind = MAP_INDEX(map_, i, j);

  if (!MAP_VALID(map_, i, j)) {
    ROS_ERROR("OccMap::shortestPath() Invalid destination: x=%f y=%f",
              stopx, stopy);
    ROS_BREAK();
    return path; // return to prevent compiler warning
  } else if ( prev_i_[ind] == -1 || prev_j_[ind] == -1) {
    return path;
  } else {
    buildPath(i, j, &path);
    return Path(path.rbegin(), path.rend());
  }
}

void OccupancyMap::setThresholds(int free, int occ) {
  if (free < 0 || free >= 100) {
    ROS_ERROR("Unoccupied space threshold must be in the range [0,100)");
    return;
  }
  if (occ <= 0 || occ > 100) {
    ROS_ERROR("Unoccupied space threshold must be in the range (0,100]");
    return;
  }
  if (free >= occ) {
    ROS_ERROR("Unoccupied space threshold must be less than occupied threshold");
    return;
  }
  max_free_threshold_ = free;
  min_occupied_threshold_ = occ;
}

} // end namespace scarab