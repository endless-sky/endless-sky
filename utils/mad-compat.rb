class MadCompat < Formula
  desc "MPEG audio decoder (custom build for Endless Sky)"
  homepage "https://www.underbit.com/products/mad/"
  url "https://downloads.sourceforge.net/project/mad/libmad/0.15.1b/libmad-0.15.1b.tar.gz"
  sha256 "bbfac3ed6bfbc2823d3775ebb931087371e142bb0e9bb1bee51a76a6e0078690"
  license "GPL-2.0-or-later"

  livecheck do
    url :stable
    regex(%r{url=.*?/libmad[._-]v?(\d+(?:\.\d+)+[a-z]?)\.t}i)
  end

  keg_only "not linked to prevent conflicts with homebrew/core libmad"

  depends_on "autoconf" => :build
  depends_on "automake" => :build
  depends_on "libtool" => :build

  def install
    # https://github.com/endless-sky/endless-sky - added for backwards compatibility
    ENV["MACOSX_DEPLOYMENT_TARGET"] = "10.9"

    touch "NEWS"
    touch "AUTHORS"
    touch "ChangeLog"
    system "autoreconf", "-fiv"
    system "./configure", "--disable-debugging", "--enable-fpm=64bit", "--prefix=#{prefix}"
    system "make", "CFLAGS=#{ENV.cflags}", "LDFLAGS=#{ENV.ldflags}", "install"
    (lib+"pkgconfig/mad.pc").write pc_file
    pkgshare.install "minimad.c"
  end

  test do
    system ENV.cc, "-I#{include}", pkgshare/"minimad.c", "-L#{lib}", "-lmad", "-o", "minimad"
    system "./minimad <#{test_fixtures("test.mp3")} >test.wav"
    assert_equal 4608, (testpath/"test.wav").size?
  end

  def pc_file
    <<~EOS
      prefix=#{opt_prefix}
      exec_prefix=${prefix}
      libdir=${exec_prefix}/lib
      includedir=${prefix}/include

      Name: mad
      Description: MPEG Audio Decoder
      Version: #{version}
      Requires:
      Conflicts:
      Libs: -L${libdir} -lmad -lm
      Cflags: -I${includedir}
    EOS
  end
end
