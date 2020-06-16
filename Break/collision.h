#pragma once

struct SegmentOverlapResult
{
    float collisionParam;
    bool overlapped;
};

struct OverlapResult
{
    vec normal;
    vec collisionPoint;
    float penetration;
};

SegmentOverlapResult test_circle_segment_overlap(vec circleCenter, float circleRadius, vec aabbTopLeft, vec aabbBottomRight)
{
    SegmentOverlapResult res;

    float paddleWidth = aabbBottomRight.x - aabbTopLeft.x;
    float distanceToPaddle = aabbTopLeft.x - circleCenter.x;

    vec toPaddle = aabbTopLeft - circleCenter;

    float a = paddleWidth * paddleWidth;
    float b = 2 * distanceToPaddle * paddleWidth;
    float c = vec_dot(toPaddle, toPaddle) - circleRadius * circleRadius;

    float collisionParam = -1.0f;
    float discriminant = b*b - 4 * a*c;
    if (discriminant >= 0)
    {
        discriminant = sqrt(discriminant);
        float t1 = (-b - discriminant) / (2 * a);
        float t2 = (-b + discriminant) / (2 * a);
        if (t1 >= 0 && t1 <= 1)
        {
            collisionParam = t1;
        }
        if (t2 >= 0 && t2 <= 1)
        {
            collisionParam = t2;
        }
    }

    res.collisionParam = collisionParam;
    if (collisionParam >= 0)
    {
        res.overlapped = true;
    }
    else
    {
        res.overlapped = false;
    }

    return res;
}

// Taken from https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331
bool test_circle_aabb_overlap(OverlapResult& result, vec circleCenter, float circleRadius, vec aabbMin, vec aabbMax, bool bTestOnly)
{
    result = {};
    vec halfExtent = (aabbMax - aabbMin) * 0.5f;
    vec aabbCenter = aabbMin + halfExtent;

    vec toCircle = circleCenter - aabbCenter;

    vec closestDistanceToCircleFromAabbCenter
    {
        clamp(toCircle.x, -halfExtent.x, halfExtent.x),
        clamp(toCircle.y, -halfExtent.y, halfExtent.y)
    };

    bool circleCenterInsideAabb = false;
    if (toCircle == closestDistanceToCircleFromAabbCenter)
    {
        circleCenterInsideAabb = true;

        // Find closest axis
        if (abs(toCircle.x) > abs(toCircle.y))
        {
            // Clamp to closest extent
            if (closestDistanceToCircleFromAabbCenter.x > 0)
                closestDistanceToCircleFromAabbCenter.x = halfExtent.x;
            else
                closestDistanceToCircleFromAabbCenter.x = -halfExtent.x;
        }

        // y axis is shorter
        else
        {
            // Clamp to closest extent
            if (closestDistanceToCircleFromAabbCenter.y > 0)
                closestDistanceToCircleFromAabbCenter.y = halfExtent.y;
            else
                closestDistanceToCircleFromAabbCenter.y = -halfExtent.y;
        }
    }

    result.normal = toCircle - closestDistanceToCircleFromAabbCenter;
    float d = vec_lengthsq(result.normal);
    float r = circleRadius;


    // Early out of the radius is shorter than distance to closest point and
    // Circle not inside the AABB
    if (d > r* r && !circleCenterInsideAabb)
        return false;

    // Avoided sqrt until we needed
    d = sqrt(d);
    result.normal /= d;

    // Collision normal needs to be flipped to point outside if circle was
    // inside the AABB
    if (circleCenterInsideAabb)
    {
        result.normal = vec_normalize(-toCircle);
        result.penetration = d - r;
    }
    else
    {
        result.penetration = r - d;
    }


    return true;
}
