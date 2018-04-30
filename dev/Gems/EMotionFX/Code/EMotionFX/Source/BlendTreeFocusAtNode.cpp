/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EMotionFXConfig.h"
#include "BlendTreeFocusAtNode.h"
#include "EventManager.h"
#include "AnimGraphManager.h"
#include "Node.h"
#include "TransformData.h"
#include "ActorInstance.h"
#include "ConstraintTransformRotationAngles.h"

#include <MCore/Source/Compare.h>
#include <MCore/Source/AttributeSettings.h>
#include <MCore/Source/AttributeVector2.h>


namespace EMotionFX
{
    // constructor
    BlendTreeFocusAtNode::BlendTreeFocusAtNode(AnimGraph* animGraph)
        : AnimGraphNode(animGraph, nullptr, TYPE_ID)
    {
        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();
    }


    // destructor
    BlendTreeFocusAtNode::~BlendTreeFocusAtNode()
    {
    }


    // create
    BlendTreeFocusAtNode* BlendTreeFocusAtNode::Create(AnimGraph* animGraph)
    {
        return new BlendTreeFocusAtNode(animGraph);
    }


    // create unique data
    AnimGraphObjectData* BlendTreeFocusAtNode::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // register the ports
    void BlendTreeFocusAtNode::RegisterPorts()
    {
        // setup the input ports
        InitInputPorts(5);
        SetupInputPort("Pose",     INPUTPORT_POSE,     AttributePose::TYPE_ID,             PORTID_INPUT_POSE);
        SetupInputPort("Offset Pos", INPUTPORT_OFFSETPOS,  MCore::AttributeVector3::TYPE_ID,   PORTID_INPUT_OFFSETPOS);
        SetupInputPort("Reference Pos", INPUTPORT_REFERENCEPOS,  MCore::AttributeVector3::TYPE_ID,   PORTID_INPUT_REFERENCEPOS);
        SetupInputPort("Goal Pos", INPUTPORT_GOALPOS,  MCore::AttributeVector3::TYPE_ID,   PORTID_INPUT_GOALPOS);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void BlendTreeFocusAtNode::RegisterAttributes()
    {
        // the node
        MCore::AttributeSettings* attrib = RegisterAttribute("Node", "node", "The node to apply the lookat to. For example the hand.", ATTRIBUTE_INTERFACETYPE_NODESELECTION);
        attrib->SetDefaultValue(MCore::AttributeString::Create());
        attrib->SetReinitObjectOnValueChange(true);

        // chain length
        attrib = RegisterAttribute("Node", "node", "The node to apply the lookat to. For example the hand.", ATTRIBUTE_INTERFACETYPE_INTSLIDER);
        attrib->SetDefaultValue(MCore::AttributeInt32::Create(3));
        attrib->SetMinValue(MCore::AttributeInt32::Create(1));
        attrib->SetMaxValue(MCore::AttributeInt32::Create(5));

        // minimum rotation angles
        attrib = RegisterAttribute("Yaw/Pitch Min", "limitsMin", "The minimum rotational yaw and pitch angle limits, in degrees.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2);
        attrib->SetDefaultValue(MCore::AttributeVector2::Create(-90.0f, -50.0f));
        attrib->SetMinValue(MCore::AttributeVector2::Create(-90.0f, -90.0f));
        attrib->SetMaxValue(MCore::AttributeVector2::Create(90.0f, 90.0f));

        // maximum rotation angles
        attrib = RegisterAttribute("Yaw/Pitch Max", "limitsMax", "The maximum rotational yaw and pitch angle limits, in degrees.", MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2);
        attrib->SetDefaultValue(MCore::AttributeVector2::Create(90.0f, 30.0f));
        attrib->SetMinValue(MCore::AttributeVector2::Create(-90.0f, -90.0f));
        attrib->SetMaxValue(MCore::AttributeVector2::Create(90.0f, 90.0f));

        // constraint rotation
        attrib = RegisterAttribute("Constraint Rotation", "constraintRotation", "A rotation that rotates the constraint space.", ATTRIBUTE_INTERFACETYPE_ROTATION);
        attrib->SetDefaultValue(AttributeRotation::Create(0.0f, 0.0f, 0.0f));

        // post rotation
        attrib = RegisterAttribute("Post Rotation", "postRotation", "The relative rotation applied after solving.", ATTRIBUTE_INTERFACETYPE_ROTATION);
        attrib->SetDefaultValue(AttributeRotation::Create(0.0f, 0.0f, 0.0f));

        // follow speed
        attrib = RegisterAttribute("Follow Speed", "speed", "The speed factor at which to follow the goal. A value near zero meaning super slow and a value of 1 meaning instant following.", MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0.75f));
        attrib->SetMinValue(MCore::AttributeFloat::Create(0.05f));
        attrib->SetMaxValue(MCore::AttributeFloat::Create(1.0));

        // twist/roll axis
        attrib = RegisterAttribute("Roll Axis", "twistAxis", "The axis used for twist/roll.", MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX);
        attrib->ResizeComboValues(3);
        attrib->SetComboValue(0, "X-Axis");
        attrib->SetComboValue(1, "Y-Axis");
        attrib->SetComboValue(2, "Z-Axis");
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));

        // enable constraints?
        attrib = RegisterAttribute("Enable Limits", "limitsEnabled", "Enable rotational limits?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(0));
        attrib->SetReinitGuiOnValueChange(true);
    }


    // get the palette name
    const char* BlendTreeFocusAtNode::GetPaletteName() const
    {
        return "FocusAt";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFocusAtNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_CONTROLLERS;
    }


    // create a clone of this node
    AnimGraphObject* BlendTreeFocusAtNode::Clone(AnimGraph* animGraph)
    {
        // create the clone
        BlendTreeFocusAtNode* clone = new BlendTreeFocusAtNode(animGraph);

        // copy base class settings such as parameter values to the new clone
        CopyBaseObjectTo(clone);

        // return a pointer to the clone
        return clone;
    }


    // init the node (pre-alloc data)
    void BlendTreeFocusAtNode::Init(AnimGraphInstance* animGraphInstance)
    {
        MCORE_UNUSED(animGraphInstance);
    }


    // pre-create unique data object
    void BlendTreeFocusAtNode::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data for this node, if it doesn't exist yet, create it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        uniqueData->mMustUpdate = true;
        UpdateUniqueData(animGraphInstance, uniqueData);
    }


    // the main process method of the final node
    void BlendTreeFocusAtNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // make sure we have at least an input pose, otherwise output the bind pose
        if (GetInputPort(INPUTPORT_POSE).mConnection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // get the weight
        float weight = 1.0f;
        if (GetInputPort(INPUTPORT_WEIGHT).mConnection)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_WEIGHT));
            weight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);
            weight = MCore::Clamp<float>(weight, 0.0f, 1.0f);
        }

        // if the weight is near zero, we can skip all calculations and act like a pass-trough node
        if (weight < MCore::Math::epsilon || mDisabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            *outputPose = *inputPose;
            UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
            UpdateUniqueData(animGraphInstance, uniqueData); // update the unique data (lookup node indices when something changed)
            uniqueData->mFirstUpdate = true;
            return;
        }

        // get the input pose and copy it over to the output pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        *outputPose = *inputPose;

        //------------------------------------
        // get the node indices to work on
        //------------------------------------
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        UpdateUniqueData(animGraphInstance, uniqueData); // update the unique data (lookup node indices when something changed)
        if (uniqueData->mIsValid == false)
        {
        #ifdef EMFX_EMSTUDIOBUILD
            uniqueData->mMustUpdate = true;
            UpdateUniqueData(animGraphInstance, uniqueData);

            if (uniqueData->mIsValid == false)
            {
                SetHasError(animGraphInstance, true);
            }
        #endif
            return;
        }

        // there is no error
    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        // get the offset pos
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_OFFSETPOS));
        const MCore::AttributeVector3* inputOffsetPos = GetInputVector3(animgraphInstance, INPUTPORT_OFFSETPOS);
        AZ::Vector3 offsetPos = (inputOffsetPos) ? AZ::Vector3(inputOffsetPos->GetValue()) : AZ::Vector3::CreateZero();

        // get the reference pos
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_REFERENCEPOS));
        const MCore::AttributeVector3* inputReferencePos = GetInputVector3(animgraphInstance, INPUTPORT_REFERENCEPOS);
        AZ::Vector3 referencePos = (inputReferencePos) ? AZ::Vector3(inputReferencePos->GetValue()) : AZ::Vector3::CreateZero();

        // get the goal
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_GOALPOS));
        const MCore::AttributeVector3* inputGoalPos = GetInputVector3(animGraphInstance, INPUTPORT_GOALPOS);
        AZ::Vector3 goal = (inputGoalPos) ? AZ::Vector3(inputGoalPos->GetValue()) : AZ::Vector3::CreateZero();

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get a shortcut to the local transform object
        const uint32 nodeIndex = uniqueData->mNodeIndex;

        Pose& outTransformPose = outputPose->GetPose();
        Transform globalTransform = outTransformPose.GetGlobalTransformInclusive(nodeIndex);

        Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();

        // << Compute angle >>
        auto referenceDirection = referencePos - offsetPos;
        auto goalDirection = goal - offsetPos;

        // MCore::Matrix referenceLookAt;
        // referenceLookAt.LookAt(offsetPos, referencePos, AZ::Vector3(0.0f, 0.0f, 1.0f));
        // MCore::Quaternion referenceDirectionRot = referenceLookAt.Transposed();
        // referenceLookAt.LookAt(offsetPos, goal, AZ::Vector3(0.0f, 0.0f, 1.0f));
        // MCore::Quaternion goalDirectionRot = referenceLookAt.Transposed();

        // MCore::Quaternion deltaRotation;
        const MCore::Quaternion deltaRotation = MCore::Quaternion::CreateDeltaRotation(referenceDirection, goalDirection);

        // << Spread angle delta between bones >>
        {
            // const AZ::Vector3 axis(0.0f, 0.0f, 1.0f);
            // const auto angle = (weight - 0.5f) * 180.0f;
            const auto deltaAngles = (deltaRotation.ToEuler() * weight) / 3;
            // const auto pitch = deltaAngles.GetX();
            // const auto yaw = deltaAngles.GetY();
            // const auto roll = deltaAngles.GetZ();
            // const auto actualDiffQuat = MCore::Quaternion(pitch, yaw, roll);
            const auto correctionMatrix = MCore::Matrix::RotationMatrixEulerXYZ(deltaAngles);
            auto nodeIdx = nodeIndex;
            auto node = skeleton->FindNodeByID(nodeIdx);
            for (size_t i = 0; i < 3 && node; i++)
            {
                Transform globalTransform = outTransformPose.GetGlobalTransformInclusive(nodeIdx);
                globalTransform.InitFromMatrix(correctionMatrix * globalTransform.ToMatrix());

                outTransformPose.SetGlobalTransformInclusive(nodeIdx, globalTransform);
                outTransformPose.UpdateLocalTransform(nodeIdx);

                nodeIdx = head->GetParentIndex();
                node = head->GetParentNode();
            }
        }

        // }
        
        return;

        // Prevent invalid float values inside the LookAt matrix construction when both position and goal are the same
        const AZ::Vector3 diff = globalTransform.mPosition - goal;
        if (diff.GetLengthSq() < AZ::g_fltEps)
        {
            goal += AZ::Vector3(0.0f, 0.000001f, 0.0f);
        }

        // calculate the lookat transform
        // TODO: a quaternion lookat function would be nicer, so that there are no matrix operations involved
        MCore::Matrix lookAt;
        lookAt.LookAt(globalTransform.mPosition, goal, AZ::Vector3(0.0f, 0.0f, 1.0f));
        MCore::Quaternion destRotation = lookAt.Transposed();   // implicit matrix to quat conversion

        // apply the post rotation
        MCore::Quaternion postRotation = static_cast<AttributeRotation*>(GetAttribute(ATTRIB_POSTROTATION))->GetRotationQuaternion();
        destRotation = destRotation * postRotation;

        // get the constraint rotation
        const MCore::Quaternion constraintRotation = static_cast<AttributeRotation*>(GetAttribute(ATTRIB_CONSTRAINTROTATION))->GetRotationQuaternion();

        if (GetAttributeFloatAsBool(ATTRIB_CONSTRAINTS))
        {
            // calculate the delta between the bind pose rotation and current target rotation and constraint that to our limits
            const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
            MCore::Quaternion parentRotationGlobal;
            MCore::Quaternion bindRotationLocal;
            if (parentIndex != MCORE_INVALIDINDEX32)
            {               
                parentRotationGlobal = inputPose->GetPose().GetGlobalTransformInclusive(parentIndex).mRotation;
                bindRotationLocal    = actorInstance->GetTransformData()->GetBindPose()->GetLocalTransform(parentIndex).mRotation;
            }
            else
            {
                parentRotationGlobal.Identity();
                bindRotationLocal.Identity();
            }

            const MCore::Quaternion destRotationLocal = destRotation * parentRotationGlobal.Conjugated();
            const MCore::Quaternion deltaRotLocal     = destRotationLocal * bindRotationLocal.Conjugated();

            // setup the constraint and execute it
            // the limits are relative to the bind pose in local space
            const AZ::Vector2 minLimits = static_cast<MCore::AttributeVector2*>(GetAttribute(ATTRIB_YAWPITCHROLL_MIN))->GetValue();
            const AZ::Vector2 maxLimits = static_cast<MCore::AttributeVector2*>(GetAttribute(ATTRIB_YAWPITCHROLL_MAX))->GetValue();
            ConstraintTransformRotationAngles constraint;   // TODO: place this inside the unique data? would be faster, but takes more memory, or modify the constraint to support internal modification of a transform directly
            constraint.SetMinRotationAngles(minLimits);
            constraint.SetMaxRotationAngles(maxLimits);
            constraint.SetMinTwistAngle(0.0f);
            constraint.SetMaxTwistAngle(0.0f);
            constraint.SetTwistAxis(static_cast<ConstraintTransformRotationAngles::EAxis>(GetAttributeFloatAsUint32(ATTRIB_TWISTAXIS)));
            constraint.GetTransform().mRotation = (deltaRotLocal * constraintRotation.Conjugated());
            constraint.Execute();

            #ifdef EMFX_EMSTUDIOBUILD
                if (GetCanVisualize(animGraphInstance))
                {
                    MCore::Matrix offset = (postRotation.Inversed() * bindRotationLocal * constraintRotation * parentRotationGlobal).ToMatrix();
                    offset.SetTranslation(globalTransform.mPosition);
                    constraint.DebugDraw(offset, GetVisualizeColor(), 0.5f);
                }
            #endif

            // convert back into global space
            destRotation = (bindRotationLocal * (constraint.GetTransform().mRotation * constraintRotation)) * parentRotationGlobal;
        }

        // init the rotation quaternion to the initial rotation
        if (uniqueData->mFirstUpdate)
        {
            uniqueData->mRotationQuat = destRotation;
            uniqueData->mFirstUpdate = false;
        }

        // interpolate between the current rotation and the destination rotation
        const float speed = GetAttributeFloat(ATTRIB_SPEED)->GetValue() * uniqueData->mTimeDelta * 10.0f;
        if (speed < 1.0f)
        {
            uniqueData->mRotationQuat = uniqueData->mRotationQuat.Slerp(destRotation, speed);
        }
        else
        {
            uniqueData->mRotationQuat = destRotation;
        }

        uniqueData->mRotationQuat.Normalize();
        globalTransform.mRotation = uniqueData->mRotationQuat;

        // only blend when needed
        if (weight < 0.999f)
        {
            outTransformPose.SetGlobalTransformInclusive(nodeIndex, globalTransform);
            outTransformPose.UpdateLocalTransform(nodeIndex);

            const Pose& inputTransformPose = inputPose->GetPose();
            Transform finalTransform = inputTransformPose.GetLocalTransform(nodeIndex);
            finalTransform.Blend(outTransformPose.GetLocalTransform(nodeIndex), weight);

            outTransformPose.SetLocalTransform(nodeIndex, finalTransform);
        }
        else
        {
            outTransformPose.SetGlobalTransformInclusive(nodeIndex, globalTransform);
            outTransformPose.UpdateLocalTransform(nodeIndex);
        }

        // perform some debug rendering
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                const float s = animGraphInstance->GetVisualizeScale() * actorInstance->GetVisualizeScale();

                const uint32 color = mVisualizeColor;
                GetEventManager().OnDrawLine(goal - AZ::Vector3(s, 0, 0), goal + AZ::Vector3(s, 0, 0), color);
                GetEventManager().OnDrawLine(goal - AZ::Vector3(0, s, 0), goal + AZ::Vector3(0, s, 0), color);
                GetEventManager().OnDrawLine(goal - AZ::Vector3(0, 0, s), goal + AZ::Vector3(0, 0, s), color);

                const AZ::Vector3& pos = globalTransform.mPosition;
                GetEventManager().OnDrawLine(pos, goal, mVisualizeColor);

                GetEventManager().OnDrawLine(globalTransform.mPosition, globalTransform.mPosition + globalTransform.mRotation.CalcUpAxis() * s * 50.0f, MCore::RGBA(0, 0, 255));
            }
        #endif
    }


    // get the type string
    const char* BlendTreeFocusAtNode::GetTypeString() const
    {
        return "BlendTreeFocusAtNode";
    }


    // update the unique data
    void BlendTreeFocusAtNode::UpdateUniqueData(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        // update the unique data if needed
        if (uniqueData->mMustUpdate)
        {
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            Actor* actor = actorInstance->GetActor();

            // don't update the next time again
            uniqueData->mMustUpdate = false;

            // get the node
            uniqueData->mNodeIndex  = MCORE_INVALIDINDEX32;
            uniqueData->mIsValid    = false;

            const char* nodeName = GetAttributeString(ATTRIB_NODE)->AsChar();
            if (nodeName == nullptr)
            {
                return;
            }

            const Node* node = actor->GetSkeleton()->FindNodeByName(nodeName);
            if (node == nullptr)
            {
                return;
            }

            uniqueData->mNodeIndex  = node->GetNodeIndex();
            uniqueData->mIsValid = true;
        }
    }


    // when an attribute value changes
    void BlendTreeFocusAtNode::OnUpdateAttributes()
    {
        // disable GUI items that have no influence
    #ifdef EMFX_EMSTUDIOBUILD
        // enable all attributes
        EnableAllAttributes(true);

        // if constraints are disabled, disable related settings
        if (GetAttributeFloatAsBool(ATTRIB_CONSTRAINTS) == false)
        {
            SetAttributeDisabled(ATTRIB_YAWPITCHROLL_MIN);
            SetAttributeDisabled(ATTRIB_YAWPITCHROLL_MAX);
            SetAttributeDisabled(ATTRIB_CONSTRAINTROTATION);
            SetAttributeDisabled(ATTRIB_TWISTAXIS);
        }
    #endif
    }


    // update
    void BlendTreeFocusAtNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update the weight node
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            UpdateIncomingNode(animGraphInstance, weightNode, timePassedInSeconds);
        }

        // update the goal node
        AnimGraphNode* goalNode = GetInputNode(INPUTPORT_GOALPOS);
        if (goalNode)
        {
            UpdateIncomingNode(animGraphInstance, goalNode, timePassedInSeconds);
        }

        // update the pose node
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE);
        if (inputNode)
        {
            UpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);

            // update the sync track
            uniqueData->Init(animGraphInstance, inputNode);
        }
        else
        {
            uniqueData->Clear();
        }

        uniqueData->mTimeDelta = timePassedInSeconds;
    }
} // namespace EMotionFX

