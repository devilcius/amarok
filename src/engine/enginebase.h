//Copyright: (C) 2003 Mark Kretschmann
//           (C) 2004 Max Howell, <max.howell@methylblue.com>
//License:   See COPYING

#ifndef AMAROK_ENGINEBASE_H
#define AMAROK_ENGINEBASE_H

#include "plugin/plugin.h" //baseclass

#include <sys/types.h>
#include <vector>

#include <qobject.h>       //baseclass
#include <qvaluelist.h>    //stack alloc

#include <kurl.h>

/**
 * @class Engine::Base
 * @author Mark Kretshmann
 * @author Max Howell
 *
 * This is an abstract base class that you need to derive when making your own backends.
 * It is typdefed to EngineBase for your conveniece.
 *
 * The only key thing to get right is what to return from state(), as some amaroK
 * behaviour is dependent on you returning the right state at the right time.
 *
 *   Empty   = No URL loaded and ready to play
 *   Idle    = URL ready for play, but not playing, so before AND after playback
 *   Playing = Playing a stream
 *   Paused  = Stream playback is paused
 *
 * Not returning idle when you have reached End-Of-Stream but amaroK has not told you
 * to stop would be bad because some components behave differently when the engine is
 * Empty or not. You are Idle because you still have a URL assigned.
 *
 * load( KURL ) is a key function because after this point your engine is loaded, and
 * amaroK will expect you to be able to play the URL until stop() or another load() is
 * called.
 *
 * You must handle your own media, do not rely on amaroK to call stop() before play() etc.
 *
 * At this time, emitting stateChanged( Engine::Idle ) is not necessary, otherwise you should
 * let amaroK know of state changes so it updates the UI correctly.
 *
 * Basically, reimplement everything virtual and ensure you emit stateChanged() correctly,
 * try not to block in any function that is called by amaroK, try to keep the user informed
 * with emit statusText()
 *
 * Only canDecode() needs to be thread-safe. Everything else is only called from the GUI thread.
 */

namespace Engine
{
    typedef std::vector<int16_t> Scope;

    class SimpleMetaBundle;
    class Effects;

    /**
     * You should return:
     * Playing when playing,
     * Paused when paused
     * Idle when you still have a URL loaded (ie you have not been told to stop())
     * Empty when you have been told to stop(), or an error occured and you stopped yourself
     *
     * It is vital to be Idle just after the track has ended!
     */
    enum State { Empty, Idle, Playing, Paused };
    enum StreamingMode { Socket, Signal, NoStreaming };


    class Base : public QObject, public amaroK::Plugin
    {
    Q_OBJECT

    signals:
        /** Emitted when end of current track is reached. */
        void trackEnded();

        /** Transmits status message, the message disappears after ~2s. */
        void statusText( const QString& );

        /**
         * Shows a long message in a non-invasive manner, you should prefer
         * this over KMessageBoxes, but do use KMessageBox when you must
         * interupt the user or the message is very important.
         */
        void infoMessage( const QString& );

        /** Transmits metadata package. */
        void metaData( const Engine::SimpleMetaBundle& );

        /** Signals a change in the engine's state. */
        void stateChanged( Engine::State );

        /** Shows amaroK config dialog at specified page */
        void showConfigDialog( const QCString& );

    public:
        virtual ~Base();

        /**
         * Initializes the engine. Must be called after the engine was loaded.
         * @return True if initialization was successful.
         */
        virtual bool init() = 0;

        /**
         * Determines if the engine is able to play a given URL.
         * @param url The URL of the file/stream.
         * @return True if we can play the URL.
         */
        virtual bool canDecode( const KURL &url ) const = 0;

        /**
         * Determines if current track is a stream.
         * @return True if track is a stream.
         */
        inline bool isStream() { return m_isStream; }

        /**
         * Load new track for playing.
         * @param url URL to be played.
         * @param stream True if URL is a stream.
         * @return True for success.
         */
        virtual bool load( const KURL &url, bool stream = false );

        /**
         * Load new track and start Playback. Convenience function for amaroK to use.
         * @param url URL to be played.
         * @param stream True if URL is a stream.
         * @return True for success.
         */
        bool play( const KURL &u, bool stream = false ) { return load( u, stream ) && play(); }

        /**
         * Start playback.
         * @param offset Start playing at @p msec position.
         * @return True for success.
         */
        virtual bool play( uint offset = 0 ) = 0;

        /** Stops playback */
        virtual void stop() = 0;

        /** Pauses playback */
        virtual void pause() = 0;

        /**
         * Get current engine status.
         * @return the correct State as described at the enum
         */
        virtual State state() const = 0;

        /** Get time position (msec). */
        virtual uint position() const = 0;

        /** Get track length (msec). */
        virtual uint length() const { return 0; }

        /**
         * Jump to new time position.
         * @param ms New position.
         */
        virtual void seek( uint ms ) = 0;

        /** Returns whether we are using the hardware volume mixer */
        inline bool isMixerHW() const { return m_mixer != -1; }

        /**
         * Determines whether media is currently loaded.
         * @return True if media is loaded, system is ready to play.
         */
        inline bool loaded() const { return state() != Empty; }

        inline uint volume() const { return m_volume; }
        inline bool hasEffects() const { return m_effects; }

        Effects& effects() const { return *m_effects; } //WARNING! calling when there are none will crash amaroK!

        /**
         * Fetch the current audio sample buffer.
         * @return Audio sample buffer.
         */
        virtual const Scope &scope() { return m_scope; };

        bool setHardwareMixer( bool );

        /**
         * Set new volume value.
         * @param value Volume in range 0 to 100.
         */
        void setVolume( uint value );

        /** Set new crossfade length (msec) */
        void setXfadeLength( int value ) { m_xfadeLength = value; }

        /** Set whether equalizer is enabled
          * You don't need to cache the parameters, setEqualizerParameters is called straight after this
          * function, _always_.
          */
        virtual void setEqualizerEnabled( bool ) {};

        /** Set equalizer parameters, all in range -100..100, where 0 = no adjustment
          * @param preamp the preamplification value
          * @param bandGains a list of 10 integers, ascending in frequency, the exact frequencies you amplify
          *                  are not too-important at this time
          */
        virtual void setEqualizerParameters( int /*preamp*/, const QValueList<int> &/*bandGains*/ ) {};

    protected:
        Base( Effects* = 0 );

        /** shows the amaroK configuration dialog at the engine page */
        void showEngineConfigDialog() { emit showConfigDialog( "Engine" ); }

        virtual void setVolumeSW( uint percent ) = 0;
        void setVolumeHW( uint percent );

        void setEffects( Effects *e ) { m_effects = e; }

        Base( const Base& ); //disable copy constructor
        const Base &operator=( const Base& ); //disable copy constructor

        int           m_xfadeLength;

    private:
        Effects      *m_effects;
        int           m_mixer;

    protected:
        uint  m_volume;
        KURL  m_url;
        Scope m_scope;
        bool  m_isStream;
    };


    class SimpleMetaBundle {
    public:
        QString title;
        QString artist;
        QString album;
        QString comment;
    };


    class Effects
    {
    public:
        virtual QStringList availableEffects() const = 0;
        virtual std::vector<long> activeEffects() const = 0;
        virtual QString effectNameForId( long ) const = 0;
        virtual bool effectConfigurable( long ) const = 0;
        virtual long createEffect( const QString& ) = 0;
        virtual void removeEffect( long ) = 0;
        virtual void configureEffect( long ) = 0;
    };
}

typedef Engine::Base EngineBase;

#endif
