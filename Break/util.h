template<typename T>
void unordered_delete(std::vector<T>& v, int i)
{
    if(v.size() > 1)
    {
        std::swap(v[i], v[v.size() - 1]);
    }
    v.pop_back();
}