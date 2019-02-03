#include "SoxProducer.h"

SoxProducer::SoxProducer(AudioQueue *audioQueue) : audioQueue(audioQueue)
{
    XOJ_INIT_TYPE(SoxProducer);
}

SoxProducer::~SoxProducer()
{
    XOJ_CHECK_TYPE(SoxProducer);

    XOJ_RELEASE_TYPE(SoxProducer);
}

void SoxProducer::start(std::string filename, const DeviceInfo &outputDevice, sox_uint64_t timestamp)
{
    XOJ_CHECK_TYPE(SoxProducer);

    this->inputFile = sox_open_read(filename.c_str(), nullptr, nullptr, nullptr);

    if (this->inputFile == nullptr)
    {
        g_warning("SoxConsumer: output file \"%s\" could not be opened", filename.c_str());
        return;
    }

    //TODO implement seeking (this is hard since we need to get the frame offset)

    this->producerThread = new std::thread([&]
        {
            unsigned long framesRead;
            auto *buffer = new sox_sample_t[1024];

            while(!this->stopProducer && (framesRead = sox_read(this->inputFile, buffer, 1024)))
            {
                while(this->audioQueue->size() > 4096)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
                this->audioQueue->push(buffer, framesRead);
            }
            this->audioQueue->signalEndOfStream();

            delete[] buffer;
            buffer = nullptr;
            sox_close(this->inputFile);
        });
}

sox_signalinfo_t* SoxProducer::getSignalInformation()
{
    XOJ_CHECK_TYPE(SoxProducer);

    return &this->inputFile->signal;
}

void SoxProducer::abort()
{
    XOJ_CHECK_TYPE(SoxProducer);

    this->stopProducer = true;
    // Wait for producer to finish
    stop();
    this->stopProducer = false;

}

void SoxProducer::stop()
{
    XOJ_CHECK_TYPE(SoxProducer);

    // Wait for producer to finish
    if (this->producerThread && this->producerThread->joinable())
    {
        this->producerThread->join();
    }
}