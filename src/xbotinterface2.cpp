#include <xbot2_interface/common/plugin.h>
#include <xbot2_interface/common/utils.h>

#include "impl/xbotinterface2.hxx"
#include "impl/utils.h"
#include "impl/joint.hxx"
#include "impl/load_object.h"

using namespace XBot;

XBotInterface2::XBotInterface2(const XBotInterface2::ConfigOptions &opt):
    XBotInterface2(opt.urdf, opt.srdf)
{

}

XBotInterface2::XBotInterface2(urdf::ModelConstSharedPtr urdf,
                               srdf::ModelConstSharedPtr srdf)
{
    impl = std::make_shared<Impl>(urdf, srdf, *this);
}

XBotInterface2::XBotInterface2(std::shared_ptr<Impl> _impl):
    impl(_impl)
{

}

ModelInterface2::UniquePtr ModelInterface2::getModel(urdf::ModelConstSharedPtr urdf,
                                                     srdf::ModelConstSharedPtr srdf,
                                                     std::string type)
{
    XBotInterface2::ConfigOptions opt { urdf, srdf };

    auto mdl = CallFunction<ModelInterface2*>("libmodelinterface2_" + type + ".so",
                                              "xbot2_create_model_plugin_" + type,
                                              opt);

    return UniquePtr(mdl);
}

void ModelInterface2::syncFrom(const XBotInterface2 &other)
{
    setJointPosition(other.getJointPosition());
    setJointVelocity(other.getJointVelocity());
    setJointEffort(other.getJointEffort());
    setJointAcceleration(other.getJointAcceleration());
}

urdf::ModelConstSharedPtr XBotInterface2::getUrdf() const
{
    return impl->_urdf;
}

srdf::ModelConstSharedPtr XBotInterface2::getSrdf() const
{
    return impl->_srdf;
}

bool XBotInterface2::hasRobotState(string_const_ref name) const
{
    try
    {
        getRobotState(name);
        return true;
    }
    catch(std::out_of_range& e)
    {
        return false;
    }
}

Eigen::VectorXd XBotInterface2::getRobotState(string_const_ref name) const
{
    return impl->getRobotState(name);
}

bool XBotInterface2::getRobotState(string_const_ref name, Eigen::VectorXd &q) const
{
    try
    {
        q = impl->getRobotState(name);
        return true;
    }
    catch(std::out_of_range&)
    {
        return false;
    }
}

int XBotInterface2::getJointNum() const
{
    return impl->_joints.size();
}

bool XBotInterface2::hasJoint(string_const_ref name) const
{
    return static_cast<bool>(getJoint(name));
}

ModelJoint::Ptr ModelInterface2::getJoint(string_const_ref name)
{
    return impl->getJoint(name);
}

ModelJoint::Ptr ModelInterface2::getJoint(int i)
{
    return impl->getJoint(i);
}

ModelInterface2::~ModelInterface2()
{

}

Joint::ConstPtr XBotInterface2::getJoint(string_const_ref name) const
{
    return impl->getJoint(name);
}

Joint::ConstPtr XBotInterface2::getJoint(int i) const
{
    return impl->getJoint(i);
}

JointInfo XBotInterface2::getJointInfo(string_const_ref name) const
{
    int jid = getJointId(name);

    if(jid < 0)
    {
        throw std::out_of_range("joint '" + name + "' not found");
    }

    return impl->_joint_info[jid];

}

JointInfo XBotInterface2::getJointInfo(int i) const
{
    if(i >= getJointNum())
    {
        throw std::out_of_range("joint #'" + std::to_string(i) + "' does not exist");
    }

    return impl->_joint_info[i];
}

int XBotInterface2::getJointId(string_const_ref name) const
{
    try
    {
        return impl->_name_id_map.at(name);
    }
    catch(std::out_of_range&)
    {
        return -1;
    }
}

bool XBotInterface2::getJacobian(string_const_ref link_name, MatRef J) const
{
    int idx = impl->get_link_id_error(link_name);

    if(idx < 0)
    {
        return false;
    }

    getJacobian(idx, J);

    return true;
}

bool XBotInterface2::getJacobian(string_const_ref link_name, Eigen::MatrixXd &J) const
{
    J.setZero(6, getNv());
    return getJacobian(link_name, MatRef(J));
}


void XBotInterface2::getRelativeJacobian(int distal_id, int base_id, MatRef Jrel) const
{
    // take Jrel from tmp
    auto& Jtmp = impl->_tmp.J;

    // initialize Jrel with Jbase
    getJacobian(base_id, Jtmp);

    // shift ref point to p_d
    Eigen::Affine3d w_T_b = getPose(base_id);
    Eigen::Affine3d w_T_d = getPose(distal_id);
    Eigen::Vector3d r = w_T_d.translation() - w_T_b.translation();
    Utils::changeRefPoint(Jtmp, r);

    // get distal jacobian
    getJacobian(distal_id, Jrel);

    // now Jrel = Jd - Jb_shifted
    Jrel = Jrel - Jtmp;

    // rotate to base
    Utils::rotate(Jrel, w_T_b.linear().transpose());
}

bool XBotInterface2::getRelativeJacobian(string_const_ref distal_name,
                                         string_const_ref base_name,
                                         Eigen::MatrixXd &J) const
{
    int didx = impl->get_link_id_error(distal_name);

    int bidx = impl->get_link_id_error(base_name);

    if(didx < 0 || bidx < 0)
    {
        return false;
    }

    getRelativeJacobian(didx, bidx, J);

    return true;

}

Eigen::Affine3d XBotInterface2::getPose(string_const_ref link_name) const
{
    return getPose(impl->get_link_id_throw(link_name));
}

bool XBotInterface2::getPose(string_const_ref link_name, Eigen::Affine3d &w_T_l) const
{
    int idx = impl->get_link_id_error(link_name);

    if(idx < 0)
    {
        return false;
    }

    w_T_l = getPose(idx);

    return true;
}

Eigen::Affine3d XBotInterface2::getPose(int distal_id, int base_id) const
{
    return getPose(base_id).inverse() * getPose(distal_id);
}

Eigen::Affine3d XBotInterface2::getPose(string_const_ref distal_name, string_const_ref base_name) const
{
    return getPose(base_name).inverse() * getPose(distal_name);
}

bool XBotInterface2::getPose(string_const_ref distal_name, string_const_ref base_name, Eigen::Affine3d &w_T_l) const
{
    int didx = impl->get_link_id_error(distal_name);

    int bidx = impl->get_link_id_error(base_name);

    if(didx < 0 || bidx < 0)
    {
        return false;
    }

    w_T_l = getPose(didx, bidx);

    return true;
}

Eigen::Vector6d XBotInterface2::getVelocityTwist(int link_id) const
{
    Eigen::Vector6d ret;
    getJacobian(link_id, impl->_tmp.J);
    ret.noalias() = impl->_tmp.J * getJointVelocity();
    return ret;
}

Eigen::Vector6d XBotInterface2::getVelocityTwist(string_const_ref link_name) const
{
    return getVelocityTwist(impl->get_link_id_throw(link_name));
}

bool XBotInterface2::getVelocityTwist(string_const_ref link_name, Eigen::Vector6d &v) const
{
    int idx = impl->get_link_id_error(link_name);

    if(idx < 0)
    {
        return false;
    }

    v = getVelocityTwist(idx);

    return true;
}

Eigen::Vector6d XBotInterface2::getAccelerationTwist(int link_id) const
{
    Eigen::Vector6d ret;
    getJacobian(link_id, impl->_tmp.J);
    ret.noalias() = impl->_tmp.J*getJointAcceleration() + getJdotTimesV(link_id);
    return ret;
}

Eigen::Vector6d XBotInterface2::getAccelerationTwist(string_const_ref link_name) const
{
    return getAccelerationTwist(impl->get_link_id_throw(link_name));
}

bool XBotInterface2::getAccelerationTwist(string_const_ref link_name, Eigen::Vector6d &v) const
{
    int idx = impl->get_link_id_error(link_name);

    if(idx < 0)
    {
        return false;
    }

    v = getAccelerationTwist(idx);

    return true;
}

Eigen::Vector6d XBotInterface2::getJdotTimesV(int link_id) const
{
    throw std::runtime_error(__func__ + std::string(" not implemented by base class"));
}

Eigen::Vector6d XBotInterface2::getJdotTimesV(string_const_ref link_name) const
{
    return getJdotTimesV(impl->get_link_id_throw(link_name));
}

bool XBotInterface2::getJdotTimesV(string_const_ref link_name, Eigen::Vector6d &a) const
{
    int idx = impl->get_link_id_error(link_name);

    if(idx < 0)
    {
        return false;
    }

    a = getJdotTimesV(idx);

    return true;
}

Eigen::Vector6d XBotInterface2::getRelativeVelocityTwist(int distal_id, int base_id) const
{
    // initialize vrel with Jbase
    Eigen::Vector6d vrel = getVelocityTwist(base_id);

    // shift ref point to p_d
    Eigen::Affine3d w_T_b = getPose(base_id);
    Eigen::Affine3d w_T_d = getPose(distal_id);
    Eigen::Vector3d r = w_T_d.translation() - w_T_b.translation();
    Utils::changeRefPoint(vrel, r);

    // now Jrel = Jd - Jb_shifted
    vrel = getVelocityTwist(distal_id) - vrel;

    // rotate to base
    Utils::rotate(vrel, w_T_b.linear().transpose());

    return vrel;
}

Eigen::Vector6d XBotInterface2::getRelativeVelocityTwist(string_const_ref distal_name,
                                                         string_const_ref base_name) const
{
    return getRelativeVelocityTwist(impl->get_link_id_throw(distal_name),
                                    impl->get_link_id_throw(base_name));
}

bool XBotInterface2::getRelativeVelocityTwist(string_const_ref distal_name,
                                              string_const_ref base_name,
                                              Eigen::Vector6d &v) const
{
    int didx = impl->get_link_id_error(distal_name);

    int bidx = impl->get_link_id_error(base_name);

    if(didx < 0 || bidx < 0)
    {
        return false;
    }

    v = getRelativeVelocityTwist(didx, bidx);

    return true;
}

Eigen::Vector6d XBotInterface2::getRelativeAccelerationTwist(int distal_id, int base_id) const
{
    Eigen::Vector6d v_base = getVelocityTwist(base_id);
    Eigen::Vector6d v_distal = getVelocityTwist(distal_id);

    Eigen::Vector6d v_rel = getRelativeVelocityTwist(distal_id, base_id);

    Eigen::Affine3d w_T_b = getPose(base_id);
    Eigen::Affine3d w_T_d = getPose(distal_id);

    Eigen::Vector6d a_d = getAccelerationTwist(distal_id);
    Eigen::Vector6d a_b = getAccelerationTwist(base_id);

    const Eigen::Matrix3d& w_R_b = w_T_b.linear();
    Eigen::Vector3d r = w_T_d.translation() - w_T_b.translation();
    Eigen::Vector3d b_om_base = w_R_b.transpose() * v_base.tail<3>();
    Eigen::Vector3d rdot = (v_distal - v_base).head<3>();

    Utils::rotate(v_rel, Utils::skew(b_om_base));
    Utils::changeRefPoint(a_b, r);

    Eigen::Vector6d rdot_cross_om;
    rdot_cross_om << rdot.cross(v_base.tail<3>()), 0, 0, 0;

    Eigen::Vector6d aux = a_d - a_b + rdot_cross_om;

    return -v_rel + Utils::rotate(aux, w_R_b.transpose());

}

Eigen::Vector6d XBotInterface2::getRelativeAccelerationTwist(string_const_ref distal_name,
                                                             string_const_ref base_name) const
{
    return getRelativeAccelerationTwist(impl->get_link_id_throw(distal_name),
                                        impl->get_link_id_throw(base_name));
}

bool XBotInterface2::getRelativeAccelerationTwist(string_const_ref distal_name, string_const_ref base_name, Eigen::Vector6d &v) const
{
    int didx = impl->get_link_id_error(distal_name);

    int bidx = impl->get_link_id_error(base_name);

    if(didx < 0 || bidx < 0)
    {
        return false;
    }

    v = getRelativeAccelerationTwist(didx, bidx);

    return true;
}

Eigen::Vector6d XBotInterface2::getRelativeJdotTimesV(int distal_id, int base_id) const
{
    Eigen::Vector6d v_base = getVelocityTwist(base_id);
    Eigen::Vector6d v_distal = getVelocityTwist(distal_id);

    Eigen::Vector6d v_rel = getRelativeVelocityTwist(distal_id, base_id);

    Eigen::Affine3d w_T_b = getPose(base_id);
    Eigen::Affine3d w_T_d = getPose(distal_id);

    Eigen::Vector6d a0_d = getJdotTimesV(distal_id);
    Eigen::Vector6d a0_b = getJdotTimesV(base_id);

    const Eigen::Matrix3d& w_R_b = w_T_b.linear();
    Eigen::Vector3d r = w_T_d.translation() - w_T_b.translation();
    Eigen::Vector3d b_om_base = w_R_b.transpose() * v_base.tail<3>();
    Eigen::Vector3d rdot = (v_distal - v_base).head<3>();

    Utils::rotate(v_rel, Utils::skew(b_om_base));
    Utils::changeRefPoint(a0_b, r);

    Eigen::Vector6d rdot_cross_om;
    rdot_cross_om << rdot.cross(v_base.tail<3>()), 0, 0, 0;

    Eigen::Vector6d aux = a0_d - a0_b + rdot_cross_om;

    return -v_rel + Utils::rotate(aux, w_R_b.transpose());

}

Eigen::Vector6d XBotInterface2::getRelativeJdotTimesV(string_const_ref distal_name,
                                                      string_const_ref base_name) const
{
    return getRelativeJdotTimesV(impl->get_link_id_throw(distal_name),
                                 impl->get_link_id_throw(base_name));
}

bool XBotInterface2::getRelativeJdotTimesV(string_const_ref distal_name,
                                           string_const_ref base_name,
                                           Eigen::Vector6d &v) const
{

    int didx = impl->get_link_id_error(distal_name);

    int bidx = impl->get_link_id_error(base_name);

    if(didx < 0 || bidx < 0)
    {
        return false;
    }

    v = getRelativeJdotTimesV(didx, bidx);

    return true;
}

XBotInterface2::JointParametrization XBotInterface2::get_joint_parametrization(string_const_ref jname)
{
    return impl->get_joint_parametrization(jname);
}

UniversalJoint::Ptr XBotInterface2::getUniversalJoint(string_const_ref name)
{
    return impl->getJoint(name);
}

UniversalJoint::Ptr XBotInterface2::getUniversalJoint(int i)
{
    return impl->getJoint(i);
}


void XBotInterface2::finalize()
{
    return impl->finalize();
}

XBotInterface2::~XBotInterface2()
{

}

// impl
XBotInterface2::Impl::Impl(urdf::ModelConstSharedPtr urdf,
                           srdf::ModelConstSharedPtr srdf,
                           XBotInterface2& api):
    _api(api),
    _urdf(urdf),
    _srdf(srdf)
{

}

Eigen::VectorXd XBotInterface2::Impl::getRobotState(string_const_ref name) const
{
    if(!_srdf)
    {
        throw std::out_of_range("cannot retrieve robot state: no srdf defined");
    }

    Eigen::VectorXd qrs = _qneutral;

    for(auto& gs: _srdf->getGroupStates())
    {
        if(gs.name_ != name)
        {
            continue;
        }

        for(auto [jname, jval] : gs.joint_values_)
        {
            int id = _name_id_map.at(jname);

            if(_joint_info[id].nq > 1 || jval.size() > 1)
            {
                // ignore
                continue;
            }

            qrs[_joint_info[id].iq] = jval[0];
        }

        return qrs;

    }

    throw std::out_of_range("cannot retrieve robot state: no such state");
}

UniversalJoint::Ptr XBotInterface2::Impl::getJoint(string_const_ref name) const
{
    auto it = _name_id_map.find(name);

    if(it == _name_id_map.end())
    {
        return nullptr;
    }

    int i = it->second;
    return _joints.at(i);
}

UniversalJoint::Ptr XBotInterface2::Impl::getJoint(int i) const
{
    return _joints.at(i);
}

XBotInterface2::JointParametrization
XBotInterface2::Impl::get_joint_parametrization(string_const_ref)
{
    JointParametrization ret;

    throw std::runtime_error(std::string(__func__) + " not impl");

    return ret;
}

int XBotInterface2::Impl::get_link_id_error(string_const_ref link_name) const
{
    int idx = _api.getLinkId(link_name);

    if(idx < 0)
    {
        std::cerr << "link '" << link_name << "' does not exist \n";
    }

    return idx;
}

int XBotInterface2::Impl::get_link_id_throw(string_const_ref link_name) const
{
    int idx = _api.getLinkId(link_name);

    if(idx < 0)
    {
        std::ostringstream ss;
        ss << "link '" << link_name << "' does not exist \n";
        throw std::out_of_range(ss.str());
    }

    return idx;
}

void XBotInterface2::Impl::finalize()
{
    int nj = 0;
    int nq = 0;
    int nv = 0;

    std::map<std::string, JointParametrization> jparam_map;

    auto handle_joint = [&](urdf::JointConstSharedPtr jptr)
    {
        auto jtype = jptr->type;

        auto jname = jptr->name;

        if(jtype == urdf::Joint::FIXED)
        {
            return;
        }

        // get parametrization from implementation
        auto jparam = _api.get_joint_parametrization(jname);

        // not found?
        if(jparam.info.id < 0)
        {
            throw std::runtime_error("joint " + jname + " not found in implementation");
        }

        // change impl id to our own id
        jparam.info.id = nj;

        // save id, nq, nv, iq, iv, name, whole param
        _joint_name.push_back(jname);

        _joint_info.push_back(jparam.info);

        _name_id_map[jname] = nj;

        jparam_map[jname] = jparam;

        // increment global dimensions
        nq += jparam.info.nq;
        nv += jparam.info.nv;
        nj += 1;
    };

    std::function<void(urdf::LinkConstSharedPtr)> pre_order_traversal =
            [&](urdf::LinkConstSharedPtr root)
    {
        for(auto j : root->child_joints)
        {
            handle_joint(j);
            pre_order_traversal(_urdf->getLink(j->child_link_name));
        }

    };

    // recursivley traverse urdf tree (depth-first)
    pre_order_traversal(_urdf->root_link_);

    // consistency checks
    for(int i = 0; i < nj; i++)
    {
        auto [jid, jiq, jiv, jnq, jnv] = _joint_info[i];

        if(jiq + jnq > nq)
        {
            throw std::runtime_error("joint q index consistency check failed (out of range)");
        }

        if(i < nj - 1 && jiq + jnq != _joint_info[i+1].iq)
        {
            throw std::runtime_error("joint q index consistency check failed (not consecutive)");
        }

        if(jiv + jnv > nv)
        {
            throw std::runtime_error("joint v index consistency check failed (out of range)");
        }

        if(i < nj - 1 && jiv + jnv != _joint_info[i+1].iv)
        {
            throw std::runtime_error("joint v index consistency check failed (not consecutive)");
        }
    }

    // resize state and cmd
    detail::resize(_state, nq, nv, nj);
    detail::resize(_cmd, nq, nv, nj);
    _cmd.ctrlmode.setConstant(~0);
    _qneutral.setZero(nq);

    // set neutral config
    for(int i = 0; i < nj; i++)
    {
        _qneutral.segment(_joint_info[i].iq,
                          _joint_info[i].nq) = jparam_map[_joint_name[i]].q0;
    }

    // use it to initialize cmd and state
    _cmd.qcmd = _state.qref = _state.qmot = _state.qlink = _qneutral;

    // construct internal joints
    for(int i = 0; i < nj; i++)
    {
        auto jname = _joint_name[i];
        auto jptr = _urdf->joints_.at(jname);

        auto [id, iq, iv, nq, nv] = _joint_info[i];

        // create xbot's joint

        // get views on relevant states and control
        auto sv = detail::createView(_state,
                                     iq, nq,
                                     iv, nv);

        auto cv = detail::createView(_cmd,
                                     iq, nq,
                                     iv, nv,
                                     i, 1);

        // create private implementation of joint
        auto jimpl = std::make_unique<Joint::Impl>(sv, cv, jptr);

        // inject mappings
        auto& jparam = jparam_map.at(jname);
        jimpl->fn_minimal_to_q = jparam.fn_minimal_to_q;
        jimpl->fn_q_to_minimal = jparam.fn_q_to_minimal;
        jimpl->fn_fwd_kin = jparam.fn_fwd_kin;
        jimpl->fn_inv_kin = jparam.fn_inv_kin;

        // check we got all info we need
        if(nq != nv &&
                !jimpl->fn_minimal_to_q)
        {
            throw std::runtime_error("fn_minimal_to_q not provided for non-euclidean joint " + jname);
        }

        if(nq != nv &&
                !jimpl->fn_q_to_minimal)
        {
            throw std::runtime_error("fn_q_to_minimal not provided for non-euclidean joint " + jname);
        }

        if(jptr->type == urdf::Joint::FLOATING &&
                nq != nv &&
                !jimpl->fn_fwd_kin)
        {
            throw std::runtime_error("fn_fwd_kin not provided for non-euclidean floating joint " + jname);
        }

        if(jptr->type == urdf::Joint::FLOATING &&
                nq != nv &&
                !jimpl->fn_inv_kin)
        {
            throw std::runtime_error("fn_inv_kin not provided for non-euclidean floating joint " + jname);
        }

        // create and save joint
        auto j =  std::make_shared<UniversalJoint>(std::move(jimpl));
        _joints.push_back(j);

    }

    // resize temporaries
    _tmp.setZero(nq, nv);

    // trigger model update
    _api.update();

}

void XBotInterface2::Impl::Temporaries::setZero(int nq, int nv)
{
    J.setZero(6, nv);
    Jarg.setZero(6, nv);
}

bool XBotInterface2::ConfigOptions::set_urdf(std::string urdf_string)
{
    auto urdf = std::make_shared<urdf::Model>();
    return urdf->initString(urdf_string);
}

bool XBotInterface2::ConfigOptions::set_srdf(std::string srdf_string)
{
    auto srdf = std::make_shared<srdf::Model>();
    return srdf->initString(*urdf, srdf_string);
}
