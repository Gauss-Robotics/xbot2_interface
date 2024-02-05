#ifndef XBOT2IFC_COLLISION_H
#define XBOT2IFC_COLLISION_H

#ifndef XBOT2IFC_COLLISION_SUPPORT
#error "collision support not included: did you link your library against the xbot2_interface::collision cmake target?"
#endif

#include <variant>

#include "xbotinterface2.h"

namespace XBot::Collision
{

inline namespace v2 {

struct XBOT2IFC_API Shape
{
    struct Sphere
    {
        double radius;
    };

    struct Capsule
    {
        double radius;
        double length;
    };

    struct Box
    {
        Eigen::Vector3d size;
    };

    struct Cylinder
    {
        double radius;
        double length;
    };

    struct Mesh
    {
        std::string filepath;
    };

    struct HeightMap
    {
        double dim_x;
        double dim_y;
        std::shared_ptr<Eigen::MatrixXf> height;
    };

    using Variant = std::variant<
        Sphere,
        Capsule,
        Box,
        Cylinder,
        Mesh
        >;
};

class XBOT2IFC_API CollisionModel
{

public:

    CollisionModel(ModelInterface::ConstPtr model);

    /**
     * @brief returns the number of collision pairs that the collision model will
     * compute the distance for
     * @note this is not the same as getLinkPairs().size(), as one link could
     * have an arbitrary number of collision objects associated with it
     */
    int getNumCollisionPairs() const;

    /**
     * @brief addCollisionShape
     * @param link
     * @param shape
     * @param link_T_shape
     * @return
     */
    bool addCollisionShape(string_const_ref link,
                           Shape::Variant shape,
                           Eigen::Affine3d link_T_shape);

    /**
     * @brief returns the vector of link pairs corresponding to the model's collision
     * pairs (size = getNumCollisionPairs())
     * @note link pairs could appear multiple times if they have more than one
     * collision object associated with them
     */
    std::vector<std::pair<std::string, std::string>> getCollisionPairs() const;

    /**
     * @brief returns the vector of links that are being taken into accouny by this
     * collision model
     */
    std::set<std::pair<std::string, std::string>> getLinkPairs() const;

    /**
     * @brief set the vector of links that are being taken into accouny by this
     * collision model
     * @param pairs
     */
    void setLinkPairs(std::set<std::pair<std::string, std::string>> pairs);

    /**
     * @brief checkSelfCollision
     * @return
     */
    bool checkSelfCollision(std::vector<int>& coll_pair_ids);

    /**
     * @brief checkSelfCollision
     * @return
     */
    bool checkSelfCollision();

    /**
     * @brief update the collision model with the underlying ModelInterface's state
     */
    void update();

    /**
     * @brief returns the vector of all normals, one for each collision pair;
     * the i-th normal points from the witness point on object #1 to the withess
     * point on object #2
     */
    std::vector<Eigen::Vector3d> getNormals() const;

    void getNormals(std::vector<Eigen::Vector3d>& n) const;

    /**
     * @brief return the vector of witness points, one for each collision pair,
     * expressed in world coordinates
     */
    std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>> getWitnessPoints() const;

    void getWitnessPoints(std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>>& wp) const;

    /**
     * @brief performs distance computation for all active collision pairs; if the threshold
     * parameter is greater than zero, this function will only compute cheaper approximate
     * distance between collision pairs whose distance can be proved to be above the given
     * thresold, by means of an inexpensive AABB overlap test.
     * @param threshold: min distance below which exact distance computation is performed
     * @return vector of distances, one for each collision pair
     */
    Eigen::VectorXd computeDistance(double threshold = -1) const;

    /**
     * @brief performs distance computation for all active collision pairs; if the threshold
     * parameter is greater than zero, this function will only compute cheaper approximate
     * distance between collision pairs whose distance can be proved to be above the given
     * thresold, by means of an inexpensive AABB overlap test.
     * @param threshold: min distance below which exact distance computation is performed
     * @param d (output) vector of distances, one for each collision pair
     */
    void computeDistance(Eigen::VectorXd& d, double threshold = -1) const;

    /**
     * @brief return the approximate distance Jacobian; this assumes witness points do not
     * change with configuration
     * @note it requires calling update() and computeDistance() first
     */
    Eigen::MatrixXd getDistanceJacobian() const;

    /**
     * @brief return the approximate distance Jacobian; this assumes witness points do not
     * change with configuration
     * @note it requires calling update() and computeDistance() first
     * @param (output) the distance Jacobian; size must be getNumCollisionPairs() x model->getNv()
     */
    void getDistanceJacobian(MatRef J) const;

    virtual ~CollisionModel();

private:

    class Impl;

    std::unique_ptr<Impl> impl;

};


}

}

#endif // COLLISION_H
