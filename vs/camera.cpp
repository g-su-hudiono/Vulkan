//  Copyright © 2020 Subph. All rights reserved.
//

#include "camera.h"

#include "common.h"

#define RIGHT    glm::vec3( 1.0f, 0.0f, 0.0f)
#define FRONT    glm::vec3( 0.0f, 0.0f,-1.0f)
#define WORLD_UP glm::vec3( 0.0f, 1.0f, 0.0f)
#define POSITION glm::vec3( 0.0f, 2.0f, 6.0f)
#define ZERO_VEC glm::vec3( 0.0f, 0.0f, 0.0f)

#define MAX_ZOOM  45.0f
#define MAX_PITCH 89.0f
#define SPEED     0.10f
#define SENSITIVITY   0.07f
#define VIEW_DISTANCE 1000.0f
#define VIEW_ANGLE    60.0f

#define YAW   0.0f
#define PITCH 0.0f
#define ROLL  0.0f

Camera::Camera() {
    reset();
    setPosition(POSITION);
    lookAt(ZERO_VEC);
    updateVector();
}

Camera::Camera(glm::vec3 position, glm::vec3 direction) {
    reset();
    setPosition(position);
    lookAt(direction);
    updateVector();
}

void Camera::reset() {
    axis = 1;
    lockFocus = false;
    useConstraint = true;
    maxZoom = MAX_ZOOM;
    maxPitch = MAX_PITCH;
    
    up = WORLD_UP;
    front = FRONT;
    right = RIGHT;
    worldUp = WORLD_UP;
    position = POSITION;
    focusPoint = ZERO_VEC;
    
    speed = SPEED;
    sensitivity = SENSITIVITY;
    viewDistance = VIEW_DISTANCE;
    viewAngle = VIEW_ANGLE;
    
    yaw = YAW;
    pitch = PITCH;
    roll = ROLL;
}

glm::vec3 Camera::getFront() {
    return front;
}

glm::vec3 Camera::getPosition() {
    return position;
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjection(float ratio) {
    glm::mat4 projection = glm::perspective(glm::radians(viewAngle), ratio, 0.1f, viewDistance);
    projection[1][1] *= -1; // for Vulkan, because GLM OpenGL has inverted Y clip 
    return projection;
}

void Camera::setInvertedAxis(bool value) {
    axis = value ? -1 : 1;
}

void Camera::setLockFocus(bool value) {
    lockFocus = value;
}

void Camera::setSpeed(float value) {
    speed = value;
}

void Camera::setPosition(glm::vec3 pos) {
    position = pos;
}

void Camera::lookAt(glm::vec3 focusPos) {
    focusPoint = focusPos;
    front = glm::normalize(focusPos - position);
    if (round(abs(front.y)*1000)/1000 != 1)
        right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
    updateRotation();
}

void Camera::zoom(float delta) {
    viewAngle += delta;
    viewAngle = glm::clamp(viewAngle, 1.0f, maxZoom);
}

void Camera::move(glm::vec3 direction) {
    glm::vec3 velocity = direction * speed;
    float distance = glm::distance(position, focusPoint);
    position += front * velocity.z;
    position += right * velocity.x;
    position += up    * velocity.y;
    
    if (lockFocus && direction.z == 0) adjustDistance(distance);
    if (lockFocus) lookAt(focusPoint);
}

void Camera::adjustDistance(float distance) {
    float newDistance = glm::distance(position, focusPoint);
    position = position * distance / newDistance;
}

void Camera::turn(glm::vec2 delta) {
    if (lockFocus) return;
    
    yaw   += axis * delta.x * sensitivity;
    pitch += axis * delta.y * sensitivity;
    pitch = useConstraint ? glm::clamp(pitch, -maxPitch, maxPitch) : pitch;
    updateVector();
}

void Camera::updateVector() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}

void Camera::updateRotation() {
    yaw = glm::degrees(atan2(front.z, front.x));
    pitch = glm::degrees(atan2(front.y, -front.z));
}
