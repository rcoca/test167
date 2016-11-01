#include <iostream>
#include <utility>
#include <thread>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

class timed_resolve_cmd:public std::enable_shared_from_this<timed_resolve_cmd>,private boost::noncopyable
{
    boost::asio::io_service&        ios;
    boost::asio::deadline_timer     timer;
    boost::asio::ip::tcp::resolver  resolver;
    timed_resolve_cmd(boost::asio::io_service& ios_):ios(ios_),timer(ios),resolver(ios){}
    void handle_resolve(boost::system::error_code const& ec,boost::asio::ip::tcp::resolver::iterator it)
    {
        if(ec)
        {
            std::clog<<"error: "<<ec.message()<<"\n";
            timer.cancel();
            return;
        }
        boost::asio::ip::tcp::resolver::iterator end;
        if(it!=end)
        {
            timer.cancel();
            for(;it!=end;++it)
                std::cout<<"host:"<<it->endpoint().address()<<"\n";
        }
    }
public:
    static std::shared_ptr<timed_resolve_cmd> create(boost::asio::io_service& ios)
    {
        return std::shared_ptr<timed_resolve_cmd>(new timed_resolve_cmd(ios));
    }
    bool exec(std::string const& hostname,std::string const& port)
    {
        try
        {
            timer.expires_from_now(boost::posix_time::seconds(5));
            timer.async_wait([self=shared_from_this(),this](boost::system::error_code const& ec){
               if(!ec)
                  resolver.cancel();
               std::clog<<"timer error: "<<ec.message()<<"\n";
            });
            resolver.async_resolve(boost::asio::ip::tcp::resolver::query(hostname,port),
                                   [self=shared_from_this(),this](boost::system::error_code const& ec,boost::asio::ip::tcp::resolver::iterator it)
            {
                if(ec)
                {
                    std::clog<<"resolver error: "<<ec.message()<<"\n";
                    timer.cancel();
                    return;
                }
                boost::asio::ip::tcp::resolver::iterator end;
                if(it!=end)
                {
                    timer.cancel();
                    for(;it!=end;++it)
                        std::cout<<"host:"<<it->endpoint().address()<<"\n";
                }
            });

        }
        catch(std::exception const& e)
        {
            std::clog<<e.what()<<"\n";
            return false;
        }
        return true;
    }
};

struct runner
{
    runner(std::size_t nthreads=std::thread::hardware_concurrency()):
        work(std::make_shared<boost::asio::io_service::work>(ios)),
        resolver(timed_resolve_cmd::create(ios))
    {

        std::cerr<<"Starting "<<nthreads<<" threads"<<std::endl;

        for(std::size_t i=0;i<nthreads;++i)
        {
            pool.add_thread(new boost::thread(boost::bind(&runner::run,this)));
        }

    }

    runner(const runner&) = delete;
    runner& operator=(const runner&) = delete;
    ~runner()
    {
        try
        {
            stop();
            pool.join_all();
            std::clog<<"threads joined\n";
        }
        catch(std::exception const& e)
        {
            std::cerr<<e.what()<<std::endl;
        }
        catch(...){}
    }

    void stop() { work.reset();}
    void start(std::string const& hostname,std::string const& port){resolver->exec(hostname,port);}

private:
    boost::asio::io_service                        ios;
    std::shared_ptr<boost::asio::io_service::work> work;
    boost::thread_group                            pool;
    std::shared_ptr<timed_resolve_cmd>             resolver;
    //worker method of thread pool
    void run()
    {
        try
        {
            ios.run();
        }
        catch(std::exception const& e)
        {
            std::cerr<<"run "<<e.what()<<std::endl;
        }
    }

};

int main(int argc, char* argv[])
{
    try
    {
        std::string hostname("www.google.com");
        if(argc>1)  hostname=std::string(argv[1]);

        runner Runner;
        Runner.start(hostname,"80");

    }
    catch(std::exception const& e)
    {
        std::clog<<e.what()<<std::endl;
        return 1;
    }

    return 0;
}
