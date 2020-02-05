struct SegmentOverlapResult
{
    float collisionParam;
    bool overlapped;
};

struct AabbOverlapResult
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

bool test_circle_aabb_overlap(AabbOverlapResult& result, vec circleCenter, float circleRadius, vec aabbMin, vec aabbMax, bool bTestOnly)
{
    result = {};
    vec aabbPointClosestToCircleCenter
    {
        clamp(circleCenter.x, aabbMin.x, aabbMax.x),
        clamp(circleCenter.y, aabbMin.y, aabbMax.y)
    };
    float distanceSq = vec_lengthsq(circleCenter - aabbPointClosestToCircleCenter);
    bool bOverlap = (distanceSq <= circleRadius * circleRadius);
    if (bTestOnly)
    {
        return bOverlap;
    }

    float distance = sqrt(distanceSq);
    result.penetration = distance - circleRadius;
    result.normal = circleCenter - aabbPointClosestToCircleCenter;
    result.normal /= distance;
    result.collisionPoint = aabbPointClosestToCircleCenter;

    return bOverlap;
}
