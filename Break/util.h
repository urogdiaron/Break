template<typename T>
void unordered_delete(std::vector<T>& v, typename std::vector<T>::iterator it)
{
    if(v.size() > 1)
    {
        std::iter_swap(it, --v.end());
    }
    v.pop_back();
}