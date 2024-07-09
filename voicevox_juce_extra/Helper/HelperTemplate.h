// Function to repeat elements in a vector
template<typename T>
std::vector<T> repeat(const std::vector<T>& input, const std::vector<T>& repeats) 
{
    std::vector<T> result;
    for (size_t i = 0; i < input.size(); ++i) 
    {
        result.insert(result.end(), repeats[i], input[i]);
    }
    return result;
}
