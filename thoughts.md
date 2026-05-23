
## Selecting a dir or song
I'm trying to figure out the src/ui/ui.cpp and include/ui.hpp I'm trying to figure out how to select a directory item and we kinda have that, but i don't think listDir supports absolute directories because the way it's displayed the content_list it's just song_1.mp3 or ablum2/ instead of the full path. which is what I want to show the user. I do think we pass in the curr directory into listDir which is right. But when a use selects album2/ we should append current dir to album2.

## Queues 

I'm thinking that for a queue maybe i should have a vector for it in the state structure. 

`std::vector<std::string> song_queue;`
> maybe something like this, that way we can just push/append (`.push_back`) pop (`pop`)

- i added this to it, and also removed some extra updateDirectorySelection. it was unnecessary in the way it was, also removed some input_buffer thing which was the queue idea, but the naming wasn't great and it was confusing


