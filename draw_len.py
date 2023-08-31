import ROOT as RT
from argparse import ArgumentParser as ap

if __name__ == '__main__':
  RT.gROOT.SetBatch()
  RT.gStyle.SetOptStat(0)

  parser = ap()
  parser.add_argument('-n', help='Processed Nominal file -- with weights', required=True,
                      type=str)
  parser.add_argument('-b', help='Processed Biased file', required=True,
                      type=str)
  parser.add_argument('-o', help='Output file', default='len_out.root')
  args = parser.parse_args()

  fN = RT.TFile.Open(args.n)
  fB = RT.TFile.Open(args.b)

  tN = fN.Get('tree')
  tB = fB.Get('tree')

  hN = RT.TH1D('hN', ';Trajectory Length (cm);Event Count', 100, 0, 500)
  hB = RT.TH1D('hB', '', 100, 0, 500)
  hW = RT.TH1D('hW', '', 100, 0, 500)

  tN.Draw('len>>hN')
  tB.Draw('len>>hB')
  tN.Draw('len>>hW', 'weight')

  fout = RT.TFile(args.o, 'recreate')
  c = RT.TCanvas('c', '')
  c.SetTicks()

  maxes = [h.GetMaximum() for h in [hN, hB, hW]]
  hN.SetMaximum(1.1*max(maxes))
  hB.SetLineColor(RT.kBlack)
  hW.SetLineColor(RT.kRed)

  hN.Draw()
  hB.Draw('same')
  hW.Draw('same')

  leg = RT.TLegend()
  leg.AddEntry(hN, 'Nominal', 'l')
  leg.AddEntry(hB, 'Biased', 'l')
  leg.AddEntry(hW, 'Weighted', 'l')
  leg.Draw()

  c.RedrawAxis()
  c.Write()
  fout.Close
  fN.Close()
  fB.Close()
